# C++ 关键字速查（只讲 C 里没有、或和 C 不一样的）

> 前提：你学过 C。本文只覆盖**本工程实际用到的** C++ 特性，每条都配工程里的真实例子。
> 不涉及模板、STL 容器、异常、智能指针（本工程刻意没用）。

---

## 0. 一张总表

| 关键字 / 写法 | C 里有吗 | 一句话 | 工程里的位置 |
|---|---|---|---|
| `class` / `struct` | 只有 struct | 带成员的"类型"，可以有函数 | 全工程 |
| `public` / `private` | ❌ | 访问控制 | `Stm32Uart.hpp` |
| `构造函数` / `析构函数` | ❌ | 对象创建/销毁时自动调用 | `Stm32Uart::Stm32Uart`、`~Stm32Uart` |
| `virtual` | ❌ | 允许子类改写，运行时决定调哪个 | `IUart::Transmit` |
| `= 0`（纯虚） | ❌ | 只声明不实现，类变成"接口" | 所有 `I*` 接口 |
| `override` | ❌ | 让编译器检查"确实在改写父类函数" | `Stm32Uart::Init` |
| `final` | ❌ | 不允许再被继承/改写 | `class Stm32Uart final` |
| `= default` | ❌ | 让编译器生成默认实现 | `virtual ~IUart() = default;` |
| `nullptr` | 用 `NULL` | 类型安全的空指针 | 70 处 |
| `enum class` | 只有 `enum` | 强类型枚举，不会隐式转 int | `ErrorCode` |
| `static_cast<T>` | 用强制转换 `(T)` | 有检查的转换 | 49 处 |
| `const_cast` | ❌ | 只去掉/加上 `const` | 3 处（喂给 HAL） |
| `reinterpret_cast` | 用 `(T)` | 按位重新解释指针类型 | 1 处（日志转字节） |
| `constexpr` | 用 `#define` | 编译期常量，有类型 | `kMaxUarts`、`kUartBaudrate` |
| `inline` | ✅ 有 | 允许多文件重复定义 | `FrameCodec.hpp` 里的 `Crc8` |
| `namespace` | ❌ | 名字空间，防重名 | `namespace { ... }` |
| `using X = ...` | 用 `typedef` | 类型别名 | `using RxCallback = ...` |
| 引用 `T&` | 用指针 | 别名，不能为空 | `IUart& uart_` |
| `bool` | C99 才有 `_Bool` | 真布尔 | `bool extended` |
| `sizeof` | ✅ 有 | 运算符（不是函数） | 15 处 |
| 范围 for | ❌ | 遍历数组/集合 | `for (Stm32Uart* u : g_uarts)` |

---

## 1. 类：C 的 struct + 函数 + 访问控制

C 里：

```c
typedef struct { int x; int y; } Point;
void point_move(Point* p, int dx, int dy);   // 函数和数据是分开的
```

C++ 里：

```cpp
class Point {
public:                 // 谁都能访问
    void Move(int dx, int dy);
private:                // 只有自己的成员函数能访问
    int x_ = 0;
    int y_ = 0;
};
```

工程里的 `Stm32Uart`：

```cpp
class Stm32Uart final : public IUart {
public:
    Stm32Uart(UART_HandleTypeDef* huart, uint8_t* rx_buf, uint16_t rx_size);
    ~Stm32Uart() override;
    ErrorCode Init(uint32_t baudrate) override;
    ...
private:
    UART_HandleTypeDef* huart_;
    ...
    void Deliver(uint16_t len);   // 只给自己用
};
```

**`class` 和 `struct` 的唯一区别**：`class` 默认 `private`，`struct` 默认 `public`。
工程里 `CanFrame`、`UartChunk` 用 `struct`（纯数据），`Stm32Uart` 用 `class`（有行为）。

**构造函数**：对象创建时自动跑，用来初始化成员。

```cpp
Stm32Uart::Stm32Uart(UART_HandleTypeDef* huart, uint8_t* rx_buf, uint16_t rx_size)
    : huart_(huart), rx_buf_(rx_buf), rx_size_(rx_size), rx_cb_(nullptr), rx_ctx_(nullptr) {
    // 冒号后面叫"初始化列表"，比在函数体里赋值更早、更快
}
```

**析构函数**：对象销毁时自动跑，用来释放资源（关中断、删队列）。

```cpp
UartEchoTask::~UartEchoTask() {
    if (queue_ != nullptr) {
        vQueueDelete(queue_);   // 不用手动记得删，出作用域自动调
    }
}
```

这就是 RAII 的雏形：**资源跟着对象生命周期走**。

---

## 2. 虚函数体系（本工程最核心的 C++ 特性）

这是"面向接口编程"能成立的原因，C 里只能靠函数指针表手工模拟。

### 2.1 `virtual` = 运行时决定调谁

```cpp
IUart* port = &uart1;      // 静态类型是 IUart*，实际指向 Stm32Uart
port->Transmit(data, n, 100);   // 实际调用 Stm32Uart::Transmit
```

没有 `virtual`，编译器只看指针类型（`IUart*`），会去调 `IUart::Transmit`——而它根本不存在。
有 `virtual`，就会查对象的**虚函数表**，找到真正的实现。

### 2.2 纯虚 `= 0`：只定义"合同"，不写实现

```cpp
class IUart {
public:
    virtual ErrorCode Init(uint32_t baudrate) = 0;   // 没有函数体
};
```

- 含纯虚函数的类叫**抽象类**，不能 `IUart u;` 直接实例化
- 子类必须全部实现，否则子类也是抽象类
- 效果：`IUart` 就是一份"串口必须具备哪些能力"的合同

### 2.3 `override`：让编译器帮你抓错

```cpp
ErrorCode Stm32Uart::Init(uint32_t baudrate) override;   // 写在头文件里
```

如果父类没有同名同参数的虚函数（比如手滑写成 `Init(uint16_t)`），编译器直接报错。
**没有 `override` 的话，这种错误会静默变成一个"新函数"，运行时永远调不到。**

### 2.4 `final`：到此为止

```cpp
class Stm32Uart final : public IUart { ... };
```

表示"不许再有人继承 `Stm32Uart`"。好处：编译器可以做去虚化优化，也防止误用。

### 2.5 虚析构 `virtual ~IUart() = default;`

```cpp
IUart* p = new Stm32Uart(...);
delete p;          // 若 ~IUart 不是 virtual，只会调 ~IUart，Stm32Uart 的析构不执行
```

只要一个类可能被基类指针 delete，析构就必须是 `virtual`。
`= default` 是让编译器自动生成（这里其实就是空实现）。

---

## 3. 类型安全三件套

### 3.1 `nullptr` 而不是 `NULL`

```cpp
if (huart_ == nullptr) return ErrorCode::ERR_INVALID_PARAM;
```

`NULL` 在 C 里是 `((void*)0)` 或 `0`，会有"0 到底是整数还是指针"的重载歧义。
`nullptr` 是真正的空指针类型，能隐式转成任何指针，但**不会**转成整数。

### 3.2 `enum class` 而不是 `enum`

```cpp
enum class ErrorCode : uint32_t {
    OK = 0,
    ERR_TIMEOUT = 3,
};
```

比 C 的 `enum` 多了两条：

1. **必须带作用域**：`ErrorCode::ERR_TIMEOUT`，不会和别的枚举撞名
2. **不隐式转 int**：`if (err == 3)` 编译不过，必须 `err == ErrorCode::ERR_TIMEOUT`
3. `: uint32_t` 指定底层类型（省空间 / 和协议对齐）

### 3.3 四种强制转换

C 里只有 `(T)x`，什么都拦不住。C++ 拆成四个，各有用途：

| 写法 | 用途 | 工程里的例子 |
|---|---|---|
| `static_cast<T>(x)` | 相关类型之间的转换（int↔枚举、void*↔对象指针、父↔子） | `static_cast<UartEchoTask*>(ctx)` |
| `const_cast<T>(x)` | **只**增删 `const`，别的什么都不做 | `const_cast<uint8_t*>(data)` 喂给 HAL |
| `reinterpret_cast<T>(x)` | 按位重新解释（最危险，少用） | `reinterpret_cast<const uint8_t*>(line)` 把 `char*` 当日志字节流 |
| `dynamic_cast` | 带运行时检查的向下转型（本工程没用，有 RTTI 开销） | — |

为什么 `const_cast`：`IUart::Transmit` 声明的是 `const uint8_t*`（我不改你的数据），
但 HAL 的 `HAL_UART_Transmit` 参数是 `uint8_t*`（它没加 const）。只能在这里"承诺不改"再转过去。

---

## 4. 编译期的东西

### 4.1 `constexpr` 代替 `#define`

```cpp
constexpr int kMaxUarts = 2;
constexpr uint32_t kUartBaudrate = 115200U;
```

比 `#define` 好：有类型、进符号表、能被调试器看到、作用域正常。

### 4.2 `inline` 函数放头文件

```cpp
inline uint8_t Crc8(const uint8_t* data, uint16_t len) { ... }
```

普通函数写在 `.h` 里，被两个 `.cpp` 包含就会"重复定义"链接报错。
`inline` 告诉链接器："多个 `.o` 里出现同一个，合并成一个就行"。
`FrameCodec.hpp` 整个就是 header-only，靠这个。

### 4.3 `namespace`

```cpp
namespace {
constexpr int kMaxUarts = 2;
Stm32Uart* g_uarts[kMaxUarts] = {nullptr, nullptr};
}
```

匿名 namespace = **只在本 `.cpp` 可见**，等价于 C 里的 `static` 全局变量/函数。
用它是为了不污染链接符号（多个文件都能有个 `g_uarts` 而不冲突）。

### 4.4 `using` 别名

```cpp
using RxCallback = void (*)(void* ctx, const uint8_t* data, uint16_t len);
```

比 `typedef` 好读，尤其是函数指针。有了它，接口里就能写 `StartReceive(RxCallback cb, ...)`。

### 4.5 `__has_include`（C++17）

```cpp
#if __has_include("can.h")
#  include "can.h"
#  define APP_HAS_CAN 1
#endif
```

"这个头文件存在吗？"——用来做可选功能编译（CubeMX 没配 CAN 时那套代码自动跳过）。

---

## 5. `sizeof`：运算符，不是函数

```cpp
queue_ = xQueueCreate(8, sizeof(UartChunk));   // 每条消息多大，交给 RTOS
```

三条必记：

1. **编译期就确定**，不占运行时间（变长数组除外）
2. **数组名不退化时给整个数组大小**：
   ```cpp
   uint8_t buf[64];
   sizeof(buf)      // 64
   sizeof(&buf[0])  // 4 或 8 —— 指针大小！
   ```
   所以工程里写 `sizeof(chunk.data)` 拿的是 64，而传进函数的 `uint8_t* data` 再 `sizeof` 只有指针大小。
3. **结构体可能有填充**：
   ```cpp
   struct UartChunk { uint16_t len; uint8_t data[64]; };  // sizeof = 66，对齐后 66（uint16 对齐 2）
   struct CanFrame { uint32_t id; uint8_t data[8]; uint8_t dlc; bool extended; };  // 不是 14，是 16
   ```
   所以队列消息大小一定用 `sizeof(类型)`，不要手算。

---

## 6. 引用 `&`：指针的"安全版"

```cpp
UartEchoTask(IUart& uart, BoardLed& led);   // 引用传参
```

| | 指针 `IUart*` | 引用 `IUart&` |
|---|---|---|
| 能否为空 | 能（要判空） | 不能，绑定时就必须有对象 |
| 能否改指向 | 能 | 不能 |
| 语法 | `p->Init()` | `r.Init()` |

工程里：**接口参数用引用**（`IUart& uart_`，构造时绑定，永不为空），
**可能为空的句柄用指针**（`UART_HandleTypeDef* huart_`，要判 `nullptr`）。

---

## 7. 其它小语法

**范围 for**（C 没有）：

```cpp
for (Stm32Uart* u : g_uarts) { if (u != nullptr && u->Handle() == huart) return u; }
```

**列表初始化 `{}`**：

```cpp
GPIO_InitTypeDef gpio = {0};   // 全部清零，比 memset 安全
TIM_OC_InitTypeDef oc = {0};
```

**`bool` 和 `true/false`**：`CanFrame::extended` 用它，比 `uint8_t` 表意清楚。

**函数指针**（C 也有，但 C++ 里常用 `using` + 模板替代）：

```cpp
using Callback = void (*)(void* ctx, const CanFrame& frame);
```

---

## 8. 五个最容易踩的坑

1. **基类析构不是 virtual** → `delete` 基类指针时子类资源泄漏。本工程所有接口都有 `virtual ~IXxx() = default;`。
2. **忘写 `override`** → 签名写错时静默变成新函数，运行时不生效。
3. **头文件里定义非 inline 函数/全局变量** → 多个 `.cpp` 包含就重复定义。要么 `inline`，要么放 `.cpp`。
4. **`sizeof(指针)` 当 `sizeof(数组)` 用** → 拿到 4 字节。
5. **对象切片**：`IUart u = stm32_uart;` 会把子类"切"成基类（抽象类编译不过，但换成具体类就静默出错）。本工程一律用 `IUart&` 或 `IUart*`，不按值传接口。

---

## 9. 一张速记卡

```cpp
virtual ErrorCode Init(uint32_t) = 0;   // 接口：必须有，我不实现
ErrorCode Init(uint32_t) override;      // 实现：我在改写父类
class X final : public Y { };           // 到此为止，不许继承
virtual ~X() = default;                 // 会被基类指针 delete，所以虚析构
enum class E : uint32_t { A, B };       // E::A，不会隐式转 int
static_cast<T>(x)                       // 有关系的类型转换
nullptr                                 // 空指针
constexpr int N = 8;                    // 编译期常量
inline uint8_t f(...) { }               // 放头文件不冲突
using Fn = void (*)(void*);             // 函数指针别名
```
