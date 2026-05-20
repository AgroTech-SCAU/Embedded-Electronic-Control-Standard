> 本文件用于记录 AgroTech 协会嵌入式电控开发标准的建设计划、阶段目标和后续需要补齐的内容
>
> 本计划不以 RoboMaster 电控体系为直接替代对象，而是将其长期迭代形成的模块地图、调试经验和机器人系统工程方法作为参考，逐步补齐本仓库在通用电控标准、SDK 复用、真实工程示例和协会传承路线上的不足

    Status: Draft v0.2
    用途: AgroTech 协会嵌入式电控开发标准建设计划
    目标: 建立可复用、可迁移、可训练、可维护的协会级电控技术底座
    原则: 先安全，后功能；先接口，后实现；先最小闭环，后复杂系统；先真实验证，后推广复用

---

# 1. 架构设计与框架抽象

## 1.1 建设目标

本仓库的核心目标不是为某一个项目提供一次性代码模板，而是建立一套可长期维护的协会级嵌入式电控开发标准

该标准应同时服务以下几类项目：

    1. 单设备控制项目
       例如单电机、舵机、RGB 灯、传感器节点、通信模块
    
    2. 多执行器控制项目
       例如四舵轮底盘、机械臂、夹爪、升降机构、抛秧机构、转运机构
    
    3. 整车/整机系统项目
       例如农业移动机器人、果蔬转运机器人、采摘机器人、无人农机感知控制节点
    
    4. 跨平台复用项目
       例如 STM32、Renesas RA、达妙 H7、ESP32、Linux 边缘控制器之间迁移通用 SDK

最终目标是使协会成员在不同项目中保持统一的开发方式：

    目录结构一致
    模块边界清晰
    接口风格统一
    底层平台可替换
    SDK 可复用
    测试流程可追踪
    安全策略可检查
    经验文档可传承

## 1.2 与 RoboMaster 长期迭代体系的关系

RoboMaster 强队的电控体系具有以下优势：

    1. 内容丰富
       覆盖电机、底盘、云台、发射、裁判系统、功率控制、超级电容、自瞄、导航等完整机器人模块
    
    2. 经验深厚
       大量内容来自比赛问题，例如小陀螺走偏、云台响应差、功率超限、卡弹、通信离线、底盘漂移等
    
    3. 训练路径完整
       从基础外设、电机控制、PID、多环控制逐步过渡到整车系统开发
    
    4. 真实工程验证充分
       多数模块经过长期队伍传承、实车调试和比赛验证

本仓库相对 RoboMaster 体系的定位差异如下：

    RoboMaster 体系：围绕比赛机器人长期迭代出的实战工程经验库
    本仓库：面向农业机器人与协会多项目复用的通用嵌入式电控标准

因此，本仓库不直接照搬 RoboMaster 代码结构，而是吸收其模块地图和工程经验，并转化为更通用的电控标准：

    将 RM 的底盘经验转化为通用移动底盘控制经验
    将 RM 的云台经验转化为通用姿态/执行机构控制经验
    将 RM 的发射机构经验转化为通用卡滞检测、热量/能量限制和状态机经验
    将 RM 的裁判系统经验转化为通用协议解析、双缓冲接收、CRC 和链路状态管理经验
    将 RM 的超级电容经验转化为通用电源管理、功率限制和能量缓冲经验
    将 RM 的自瞄/导航经验转化为通用上位机通信、控制权仲裁和外部系统接入经验

## 1.3 现有架构需要保持的优势

本仓库当前采用：

    src/
    ├── app/        # 应用层：任务入口、业务流程、状态机编排、整机逻辑
    ├── service/    # 服务层：组合 device + domain + infra，形成系统能力
    ├── device/     # 设备层：真实设备 SDK、设备协议、反馈缓存、设备抽象
    ├── domain/     # 领域层：运动学、控制模型、机构解算、纯算法模型
    ├── infra/      # 基础设施层：PID、滤波、矩阵、协议解析、HFSM、CRC、容器
    └── platform/   # 平台层：芯片/HAL/FSP/CubeMX 外设适配、CAN/UART/PWM/GPIO/Tick

该结构应继续保持

相比常见 `bsp/module/app` 三层结构，本仓库的主要优势是：

    1. domain 层独立
       便于沉淀机械臂运动学、舵轮运动学、底盘模型、轨迹规划、坐标变换等纯算法模块
    
    2. device 与 platform 分离
       便于同一设备协议在 STM32、Renesas、达妙板、Linux mock 环境中复用
    
    3. service 作为系统能力组合层
       便于将设备、算法、安全策略和控制周期组合成可被 app 调用的稳定接口
    
    4. infra 作为基础设施层
       便于复用 PID、滤波器、解析器、状态机、日志、错误码、容器等通用能力
    
    5. platform 作为唯一硬件平台出口
       避免 HAL/FSP/CubeMX 句柄污染上层模块

需要继续坚持的核心规则：

    下层不得依赖上层
    app 不直接操作 HAL/FSP/CubeMX 句柄
    app 不直接拼 CAN/UART/SPI 帧
    device public header 不暴露芯片平台头文件
    app 只依赖 service
    device 只依赖 infra
    domain 只依赖 infra
    infra 不依赖任何项目层；如需外部能力，由 service 注入
    platform 是唯一允许直接包含芯片 HAL/FSP/CMSIS/CubeMX 头文件的层

## 1.4 需要补齐的架构能力

当前标准已经具备基础分层和部分 SDK，但仍需要补齐以下架构能力

### 1.4.1 轻量模式与完整模式

为降低新人学习成本，应在标准中明确两种使用方式

轻量模式适用于单设备、小作业、快速验证：

    app
    device
    platform
    infra 可选

典型调用链：

    app → device → platform

最低要求：

    必须有 stop/disable
    必须有限幅
    必须有 timeout
    必须有低速测试记录
    不允许 app 直接操作芯片平台句柄

完整模式适用于底盘、机械臂、整车、多设备系统：

    app
    service
    device
    domain
    infra
    platform

典型调用链：

    app → service → device/domain/infra
                  ↘ platform 注入 device/infra

完整模式要求：

    service 统一组合系统能力
    device 统一处理真实设备协议
    domain 统一处理模型与算法
    infra 统一处理通用工具
    platform 统一处理芯片外设适配
    safety/fault/watchdog 必须纳入设计

计划事项：

    [x] 在通用开发流文档中增加轻量模式说明
    [x] 在通用开发流文档中增加完整模式说明
    [x] 给出轻量模式最小电机示例
    [x] 给出完整模式四舵轮底盘示例

### 1.4.2 service 层约束规则

service 层是当前架构中最容易膨胀的一层，需要明确边界

service 允许：

    组合 device/domain/infra
    完成 device PortOps 绑定
    维护系统状态缓存
    实现周期 update
    实现 timeout/fault/stop/brake
    实现安全限幅和降级输出
    对 app 暴露稳定系统能力

service 不允许：

    不写复杂数学模型，复杂模型放 domain
    不写真实设备协议，设备协议放 device
    不写 HAL/FSP/CubeMX 细节，平台细节放 platform
    不写整机任务流程，整机流程放 app/HFSM
    不写大量通信帧解析，通信协议放 infra/parser 或 communication_service
    不变成纯转发层，也不变成万能垃圾桶

计划事项：

    [x] 在通用开发流文档中增加 service 层约束规则
    [x] 为 chassis_service 写出正例和反例
    [x] 为 motor_service 写出正例和反例
    [x] 为 communication_service 写出正例和反例

### 1.4.3 PortOps 统一规范

PortOps 是本标准实现平台解耦的核心机制

需要统一规定：

    PortOps 应作为 init config 的一部分传入
    避免 register_port() 与 init() 分离导致漏绑定
    device/infra 不直接持有 HAL/FSP/CubeMX 句柄
    PortOps 函数指针命名应表达能力，而不是表达平台
    PortOps 返回值应统一使用 bool 或模块状态码
    PortOps 中断回调不得执行复杂控制逻辑

典型 PortOps 能力：

    send
    write
    read
    now_ms
    lock
    unlock
    delay_ms
    register_rx
    set_pwm
    set_gpio
    get_adc

计划事项：

    [ ] 定义 PortOps 命名规范
    [ ] 定义 PortOps 生命周期规范
    [ ] 定义 PortOps 回调规范
    [ ] 增加 CAN PortOps 示例
    [ ] 增加 UART DMA PortOps 示例
    [ ] 增加 PWM/GPIO PortOps 示例

### 1.4.4 错误码、状态码与生命周期统一

当前 SDK 需要进一步统一状态语义

建议所有正式 SDK 至少包含以下生命周期能力：

    init
    enable/start
    disable/stop
    update
    reset
    get_state
    clear_fault

常见状态建议统一为：

    UNINIT
    INITED
    READY
    RUNNING
    STOPPED
    FAULT
    TIMEOUT

常见返回值建议统一为：

    OK
    ERROR
    INVALID_PARAM
    NOT_INITED
    TIMEOUT
    BUS_ERROR
    DEVICE_ERROR
    OUT_OF_RANGE
    BUSY
    UNSUPPORTED

计划事项：

    [ ] 定义 `infra/status` 或 `infra/error_code`
    [x] 明确状态码是否按模块独立定义，还是提供统一基础码
    [x] 统一 SDK 中 bool、status、error 的使用边界
    [x] 增加错误码到字符串的 X-Macro 示例

### 1.4.5 safety/fault/watchdog 独立化

真实电控项目不应将安全逻辑散落在业务代码中

需要补齐以下基础能力：

    timeout_guard
    watchdog
    fault_manager
    emergency_stop
    output_limiter
    safe_state
    fault_latch
    fault_clear

故障分级建议：

    INFO       # 记录但不影响运行
    WARNING    # 提示并可能轻微限幅
    LIMIT      # 限制输出
    DEGRADE    # 降级运行
    STOP       # 停止相关执行器
    LOCK       # 锁定等待人工处理
    RESET      # 尝试重新初始化模块

计划事项：

    [ ] 增加 `sdks/infra/timeout_guard`
    [ ] 增加 `sdks/infra/watchdog`
    [ ] 增加 `sdks/infra/fault_manager`
    [ ] 在 README 安全提醒中引用 fault/watchdog 最小要求
    [ ] 在单电机示例中加入反馈超时保护
    [ ] 在四舵轮示例中加入电机离线、遥控器离线、急停保护

### 1.4.6 参数管理规范

真实项目中参数散落会严重影响调试与维护

需要规范以下参数类型：

    电机 ID
    电机方向
    编码器零位
    舵轮安装位置
    底盘尺寸
    轮半径
    PID 参数
    斜坡参数
    限幅参数
    timeout 参数
    fault 阈值
    通信频率
    控制周期

参数来源建议分层：

    编译期默认参数
    项目级 config
    设备级 config
    service init config
    Flash/EEPROM 持久化参数
    上位机临时调参参数

计划事项：

    [ ] 建立 `config` 结构体规范
    [ ] 定义参数命名和单位规范
    [ ] 给出四舵轮底盘参数表示例
    [ ] 给出单电机参数表示例
    [ ] 后续考虑 Flash 参数存储 SDK

### 1.4.7 mock/sim/pc_test 能力

本仓库的长期价值之一是让 domain/infra/device 协议层能够脱离真实硬件测试

需要逐步支持：

    mock PortOps
    fake motor feedback
    PC 端单元测试
    domain 算法输入输出测试
    protocol parser 测试
    fault/watchdog 测试
    service 逻辑仿真

计划事项：

    [ ] 建立 `tests/` 或 `examples/test_*` 约定
    [ ] 为 protocol_parser 增加测试输入样例
    [ ] 为 steer_wheel_kine 增加输入输出样例
    [ ] 为 bus_motor 增加 mock send/feedback 示例
    [ ] 为 fault_manager 增加 fault latch/clear 测试样例

### 1.4.8 reference_projects 建设

标准仓库不能只提供 SDK，还需要提供真实参考工程

建议新增：

    reference_projects/
    ├── single_bus_motor_reference/
    ├── single_dm_motor_reference/
    ├── four_swerve_chassis_reference/
    ├── six_dof_arm_joint_reference/
    ├── uart_protocol_bridge_reference/
    └── safety_stop_reference/

参考工程要求：

    能说明使用哪些 SDK
    能说明依赖哪个 chip SDK
    能说明初始化顺序
    能说明控制周期
    能说明安全限制
    能说明测试步骤
    能说明预期现象
    能说明常见故障

计划事项：

    [ ] 先建立 reference_projects 目录结构
    [ ] 第一个参考工程选择 single_dm_motor_reference
    [ ] 第二个参考工程选择 four_swerve_chassis_reference
    [ ] 后续根据协会项目补 six_dof_arm_joint_reference

### 1.4.9 文档一致性与发布规范

标准仓库自身必须保持一致

需要检查：

    README 中提到的文件必须存在
    README 中列出的 SDK 必须与 sdks/ 实际内容一致
    文档中的接口名与代码一致
    examples 路径与实际路径一致
    状态版本与 tag/release 一致
    安全提醒与实际 SDK 能力一致

计划事项：

    [ ] 恢复并维护 plan.md
    [ ] 检查 README 中的 plan.md 路径
    [ ] 检查 README 中当前 SDK 列表
    [ ] 检查通用开发流文档中的接口示例是否与实际 SDK 接近
    [ ] 每次 release 前进行文档一致性检查

## 1.5 阶段计划

### v0.1：当前阶段

目标：建立基本标准骨架

已完成或基本完成：

    [x] README 总览
    [x] 通用开发流文档
    [x] 团队协作开发文档
    [x] app/service/device/domain/infra/platform 分层
    [x] submodule 使用说明
    [x] infra 基础模块雏形
    [x] domain 运动学模块雏形
    [x] device 设备 SDK 雏形
    [x] PortOps 方向初步确定

### v0.2：接口稳定与最小闭环

目标：让标准从文档进入可验证的最小工程闭环

计划：

    [ ] 恢复 plan.md
    [x] 补充轻量模式/完整模式
    [x] 补充 service 约束规则
    [x] 统一状态码和生命周期
    [ ] 补充 timeout_guard
    [ ] 补充 limiter/ramp/filter 基础模块
    [ ] 完成 single_dm_motor_reference
    [ ] 完成 mock PortOps 示例
    [ ] 完成测试记录模板

完成标准：

    一个新成员能够根据文档完成单电机低速安全控制
    app 不直接操作 CAN/UART/HAL
    device 不暴露平台头文件
    有 timeout/stop/limit 最小安全链路

### v0.3：底盘与多执行器能力

目标：形成第一个完整多执行器系统参考工程

计划：

    [ ] 完成 steer_wheel_kine 输入输出测试
    [ ] 补充 chassis_service 设计文档
    [ ] 完成 four_swerve_chassis_reference
    [ ] 补充速度斜坡、舵向到位门控、最短转角优化说明
    [ ] 补充遥控器/上位机命令输入接口示例
    [ ] 补充底盘安全停止、离线保护、低速测试流程

完成标准：

    四舵轮底盘能够形成从命令输入到电机输出的完整链路
    能在 mock 或真实硬件上验证运动学输出
    能记录测试过程和常见故障

### v0.4：安全、调试与工程经验沉淀

目标：让标准具备真实项目维护能力

计划：

    [ ] 完成 fault_manager
    [ ] 完成 watchdog
    [ ] 完成 log/vofa/debug 数据输出规范
    [ ] 建立故障码表
    [ ] 建立测试报告模板
    [ ] 建立硬件上电前检查表
    [ ] 建立执行器低速测试流程
    [ ] 建立安全评审 checklist

完成标准：

    任何电机/底盘/机械臂项目在上真实硬件前都能进行安全检查
    故障不只靠口头描述，而能通过日志、状态码和测试记录定位

### v0.5：协会训练路线

目标：让标准不仅能被骨干维护，也能被新人学习

计划：

    [ ] 建立 onboarding 文档
    [ ] 建立新人最短路线
    [ ] 建立基础外设到单电机的训练任务
    [ ] 建立单电机到四舵轮的训练任务
    [ ] 建立代码 review 指南
    [ ] 建立常见问题索引

完成标准：

    新成员可以按训练路线逐步进入标准体系
    骨干可以按标准要求进行 code review
    协会项目可以统一引用标准仓库

### v1.0：稳定版

目标：经过至少 2~3 个真实项目验证后发布稳定接口

发布条件：

    [ ] 至少一个单设备项目使用
    [ ] 至少一个多执行器项目使用
    [ ] 至少一个整机/机器人项目使用
    [ ] 常用 SDK API 基本稳定
    [ ] 有 release/tag
    [ ] 有迁移说明
    [ ] 有已知问题列表
    [ ] 有安全检查流程
    [ ] 有参考工程

---

# 2. 具体设备与工程经验地图

## 2.1 建设目标

本部分用于补齐本仓库目前相对 RoboMaster 长期迭代体系不足的内容

RoboMaster 体系的优势不在于某一个单独算法，而在于围绕真实机器人长期积累了大量模块和问题经验：

    电机怎么接
    反馈怎么读
    PID 怎么调
    多环怎么组织
    底盘怎么解算
    舵轮怎么适配
    功率怎么限制
    超级电容怎么管理
    云台怎么自稳
    发射机构怎么处理卡弹
    裁判系统怎么解析
    自瞄和导航怎么接入
    故障怎么检测
    调试数据怎么采集

本仓库应将这些经验转化为适合农业机器人和协会项目的设备/工程经验地图

建设原则：

    不照搬 RM 特定业务
    提炼 RM 背后的通用电控问题
    优先补真实项目最常用的设备
    每个设备都要有接口、状态、测试、故障、经验说明
    每个复杂机构都要有最小参考工程

## 2.2 基础工具与基础设施地图

### 2.2.1 已有基础设施

当前已有：

    delay
    matrix
    pid
    protocol_parser
    log
    hfsm

后续要求：

    每个 infra 模块应说明适用场景
    每个 infra 模块应有最小使用示例
    每个 infra 模块应尽量可 PC 测试
    每个 infra 模块不应依赖具体芯片平台

### 2.2.2 P0 级待补基础设施

优先补齐：

    ring_buffer
    crc
    limiter
    ramp
    low_pass_filter
    moving_average_filter
    timeout_guard
    watchdog
    fault_manager
    unit_convert
    command_dispatcher

原因：

    这些模块几乎所有真实项目都会使用
    它们能显著降低安全风险和重复造轮子成本
    它们是后续底盘、机械臂、通信、传感器系统的基础

计划事项：

    [ ] `sdks/infra/ring_buffer`
    [ ] `sdks/infra/crc`
    [ ] `sdks/infra/limiter`
    [ ] `sdks/infra/ramp`
    [ ] `sdks/infra/filter`
    [ ] `sdks/infra/timeout_guard`
    [ ] `sdks/infra/watchdog`
    [ ] `sdks/infra/fault_manager`
    [ ] `sdks/infra/command_dispatcher`

### 2.2.3 调试工具经验

需要形成统一调试经验：

    串口日志
    VOFA+ 数据绘图
    JScope/Ozone 调试
    CAN 分析仪
    逻辑分析仪
    示波器
    fault code 输出
    状态机状态输出
    PID target/feedback/output 输出

每个执行器工程至少应能输出：

    target
    feedback
    error
    controller_output
    final_output
    state
    fault_code
    update_period

计划事项：

    [ ] 补充 debug 数据命名规范
    [ ] 补充 VOFA 输出格式建议
    [ ] 补充单电机调参数据模板
    [ ] 补充舵轮底盘调试数据模板

## 2.3 电机与执行器地图

### 2.3.1 bus_motor

当前已有：

    bus_motor
    dm_bus_motor

定位：

    统一总线电机设备抽象
    屏蔽不同电机协议差异
    向 service 提供 enable/disable/command/feedback/state 能力

需要继续明确：

    bus_motor 只处理总线电机设备协议和反馈缓存
    PID/前馈/斜坡不应塞进 bus_motor
    系统级 timeout/fault 可由 service 或 fault_manager 统一处理
    电机模型和功率模型不应直接放进 device

建议拆分边界：

    device/bus_motor
        负责协议、反馈、状态、命令帧
    
    infra/controller
        负责 PID、前馈、斜坡、限幅
    
    service/motor_service
        负责绑定电机、控制周期、timeout、安全输出

计划事项：

    [ ] 检查 bus_motor 生命周期接口
    [ ] 检查 bus_motor 状态码
    [ ] 检查 dm_bus_motor 与通用 bus_motor 的边界
    [ ] 增加单电机 mock 示例
    [ ] 增加单达妙电机真实工程示例

### 2.3.2 DJI RM 电机经验地图

RM 体系中常见电机：

    M3508
    M2006
    GM6020
    C610/C620 电调

本仓库后续可提炼的通用经验：

    CAN ID 管理
    反馈帧解析
    编码器跨圈
    速度反馈滤波
    电流控制指令
    温度保护
    离线检测
    多电机分组发送
    多环 PID
    动态目标与前馈

计划事项：

    [ ] 增加 rm_bus_motor 适配计划
    [ ] 增加 RM 电机反馈结构说明
    [ ] 增加多电机 CAN 发送分组说明
    [ ] 增加编码器跨圈处理模块

### 2.3.3 达妙电机经验地图

达妙电机与 RM 电机不同，需要单独沉淀：

    MIT 模式
    位置/速度/力矩混合控制
    kp/kd 指令
    使能/失能
    零位
    电机保护
    CAN/FDCAN 协议
    上电顺序
    安全失能

计划事项：

    [ ] 完善 dm_bus_motor 文档
    [ ] 建立 single_dm_motor_reference
    [ ] 增加 MIT 模式安全说明
    [ ] 增加 kp/kd 参数限制建议
    [ ] 增加 enable/disable 上电测试流程

### 2.3.4 舵机与普通执行器

后续需要补齐：

    bus_servo
    pwm_servo
    stepper_motor
    linear_actuator
    relay_output
    pump/valve
    gripper

通用经验：

    位置限幅
    速度限幅
    力矩/电流限幅
    零位校准
    机械限位
    堵转检测
    超时保护
    安全停止

计划事项：

    [ ] 增加 bus_servo SDK
    [ ] 增加 pwm_servo SDK
    [ ] 增加 stepper_motor SDK 设计文档
    [ ] 增加 gripper_service 设计文档

## 2.4 通信与协议地图

### 2.4.1 CAN/FDCAN

需要沉淀：

    CAN ID 分配
    标准帧/扩展帧
    发送队列
    接收回调
    过滤器配置
    总线负载估算
    多设备分组发送
    总线错误处理
    feedback timeout

计划事项：

    [ ] 建立 CAN PortOps 模板
    [ ] 建立 FDCAN PortOps 模板
    [ ] 增加 CAN ID 分配建议
    [ ] 增加多电机 CAN 总线负载说明

### 2.4.2 UART/USB VCOM

需要沉淀：

    DMA 接收
    空闲中断
    ring buffer
    协议帧头
    长度字段
    CRC
    粘包/拆包
    心跳包
    超时
    上位机调参
    VOFA 数据输出

计划事项：

    [ ] 建立 UART DMA PortOps 模板
    [ ] 增加 protocol_parser 与 ring_buffer 联合示例
    [ ] 增加上位机通信 reference
    [ ] 增加 debug log 与控制数据分流说明

### 2.4.3 上位机/ROS/导航通信

农业机器人项目后续需要重点支持：

    ROS cmd_vel
    导航目标状态
    作业任务状态
    底盘反馈
    机械臂状态
    急停状态
    心跳
    时间戳
    坐标系约定

计划事项：

    [ ] 增加 navigation_bridge 设计文档
    [ ] 增加 cmd_vel 到 chassis_service 的接口设计
    [ ] 增加上位机心跳与失联保护说明
    [ ] 增加坐标系约定文档

## 2.5 传感器地图

需要逐步补齐：

    encoder
    IMU
    distance_sensor
    limit_switch
    pressure_sensor
    current_sensor
    voltage_sensor
    temperature_sensor
    load_cell
    UWB module
    GPS/RTK module

通用经验：

    标定
    零偏
    温漂
    滤波
    采样周期
    时间戳
    数据过期
    坐标系
    单位换算
    异常值剔除

优先级：

    P0: encoder, limit_switch, current_sensor, voltage_sensor
    P1: IMU, distance_sensor, load_cell
    P2: UWB, GPS/RTK, complex sensor bridge

计划事项：

    [ ] 增加 encoder SDK
    [ ] 增加 limit_switch SDK
    [ ] 增加 voltage/current sensor SDK
    [ ] 增加 IMU SDK 设计文档
    [ ] 增加 sensor timeout 与数据有效性规范

## 2.6 底盘地图

底盘是本仓库后续应重点补齐的系统级能力

### 2.6.1 通用底盘能力

所有底盘都应具备：

    速度输入
    模式管理
    运动学解算
    速度限幅
    加速度斜坡
    电机输出
    反馈缓存
    急停
    timeout
    fault
    debug 输出

通用输入：

    vx
    vy
    wz
    control_mode
    enable
    emergency_stop

通用输出：

    wheel_target
    motor_command
    chassis_state
    fault_code

计划事项：

    [ ] 建立 chassis_service 通用接口
    [ ] 建立 chassis_config 参数规范
    [ ] 建立 chassis_state 状态规范
    [ ] 建立 chassis debug 数据规范

### 2.6.2 麦轮/全向轮经验

RM 中麦轮和全向轮经验可转化为：

    运动学解算
    轮速归一化
    底盘跟随
    小陀螺
    功率限制
    速度斜坡
    操作逻辑

计划事项：

    [ ] 增加 mecanum_kine 计划
    [ ] 增加 omni_kine 计划
    [ ] 增加轮速归一化说明

### 2.6.3 舵轮经验

舵轮底盘应作为本仓库重点参考工程

需要沉淀：

    舵轮运动学
    模块位置参数
    舵向角计算
    驱动速度计算
    最短转角优化
    驱动速度反向
    舵向角度闭环
    驱动速度闭环
    舵向到位门控
    启动策略
    停止策略
    刹车策略
    零位校准
    模块离线保护
    线束/滑环注意事项

典型问题：

    舵向没到位就驱动导致漂移
    目标角度跳变导致抖动
    零位不准导致直行跑偏
    转向摩擦大导致低速震荡
    四轮接地不一致导致小陀螺走偏
    功率不足导致启动无力或保护

计划事项：

    [ ] 完善 steer_wheel_kine
    [ ] 增加最短转角优化函数
    [ ] 增加舵向到位门控逻辑
    [ ] 增加速度斜坡模块
    [ ] 增加 four_swerve_chassis_reference
    [ ] 增加舵轮调试流程文档
    [ ] 增加舵轮常见故障排查文档

### 2.6.4 阿克曼/差速/农业移动底盘

农业机器人常见底盘不只 RM 全向底盘，还包括：

    差速底盘
    四轮差速
    阿克曼底盘
    履带底盘
    拖拉机式底盘

后续需要沉淀：

    ackermann_kine
    differential_kine
    wheel_odometry
    cmd_vel adapter
    nav_bridge
    safety stop

计划事项：

    [ ] 增加 differential_kine 计划
    [ ] 增加 ackermann_kine 计划
    [ ] 增加 wheel_odometry 计划
    [ ] 增加 Nav2/cmd_vel 接入说明

## 2.7 机械臂与作业机构地图

当前已有：

    six_dof_arm_kine

后续应补齐：

    joint_limit
    trajectory_profile
    interpolation
    gripper_control
    arm_service
    emergency_stop
    homing
    soft_limit
    hard_limit
    torque/current limit

农业机器人作业机构包括：

    夹爪
    升降机构
    伸缩机构
    抛秧机构
    转运机构
    分拣机构
    传送带
    执行阀/泵

RM 发射机构的经验可转化为：

    状态机
    卡滞检测
    堵转检测
    反转恢复
    热量/能量限制
    发射/执行许可判断
    安全锁定

计划事项：

    [ ] 增加 trajectory_profile
    [ ] 增加 joint_limit
    [ ] 增加 gripper_service 设计
    [ ] 增加 actuator_jam_detect 设计
    [ ] 增加作业机构 HFSM 示例

## 2.8 功率、电源与能量管理地图

RM 功率控制经验对农业机器人同样有参考价值

需要沉淀：

    电压采样
    电流采样
    功率估计
    电机功率模型
    总功率限制
    分设备功率限制
    电池低压保护
    超级电容接口
    能量缓冲
    启动电流限制
    刹车能量冲击

优先级：

    P0: voltage/current sensor + low voltage protect
    P1: output current/velocity scale limiter
    P2: motor power model
    P3: supercap interface

计划事项：

    [ ] 增加 power_monitor SDK
    [ ] 增加 low_voltage_protect 示例
    [ ] 增加 chassis_power_limiter 设计文档
    [ ] 增加 motor_power_model 研究计划
    [ ] 增加 supercap interface 计划

## 2.9 云台、姿态与指向机构地图

RM 云台经验可转化为通用姿态/指向机构经验

适用对象：

    传感器云台
    相机云台
    喷药/喷洒指向机构
    激光/测距指向机构
    机械臂末端姿态微调机构

需要沉淀：

    yaw/pitch 控制
    角度环
    速度环
    IMU 反馈
    姿态解算
    重力补偿
    摩擦补偿
    加速度补偿
    机械限位
    自稳

计划事项：

    [ ] 增加 gimbal_service 设计文档
    [ ] 增加 yaw_pitch_kinematics 计划
    [ ] 增加 IMU + gimbal 最小示例计划
    [ ] 增加姿态限位与安全停止说明

## 2.10 裁判系统经验的通用化

RM 裁判系统本身不适用于农业机器人，但其中的通信经验具有通用价值

可转化内容：

    用户协议解析
    双缓冲接收
    粘包/拆包
    CRC8/CRC16
    帧状态机
    数据有效性
    通信心跳
    UI/状态显示
    业务数据同步

计划事项：

    [ ] 增加 protocol_frame_parser 示例
    [ ] 增加 CRC SDK
    [ ] 增加双缓冲接收示例
    [ ] 增加通信心跳与失联保护说明

## 2.11 自瞄与导航经验的通用化

RM 自瞄/导航经验可转化为外部上位机系统接入经验

农业机器人可能接入：

    ROS2/Nav2
    FAST-LIO/SLAM
    视觉识别上位机
    深度相机处理节点
    任务规划器
    遥控/上位机 GUI

需要沉淀：

    外部速度指令
    外部目标指令
    心跳
    时间戳
    坐标系
    控制权仲裁
    手动抢占
    自动模式降级
    通信离线停车
    状态回传

计划事项：

    [ ] 增加 command_mux 设计
    [ ] 增加 manual/auto/navigation 控制权仲裁说明
    [ ] 增加 cmd_vel 接入底盘参考流程
    [ ] 增加外部系统心跳失联保护

## 2.12 工程经验库建设

本仓库后续需要逐渐沉淀问题库，而不只是接口库

每个复杂模块应包含：

    模块定位
    适用场景
    不适用场景
    接口说明
    参数说明
    初始化顺序
    update 周期
    安全要求
    最小测试
    常见故障
    调试数据
    参考工程

优先建立的问题库：

    单电机不转
    电机方向反
    CAN 收不到反馈
    反馈超时
    电机低速抖动
    舵轮启动漂移
    舵轮停止漂移
    舵轮目标角跳变
    遥控器离线
    串口粘包
    协议 CRC 错误
    底盘直行跑偏
    急停后电机仍有输出
    app 直接调用 HAL 导致模块无法移植

计划事项：

    [ ] 新增 `docs/engineering_notes/` 或 `examples/troubleshooting/`
    [ ] 增加单电机常见问题
    [ ] 增加 CAN/FDCAN 常见问题
    [ ] 增加舵轮底盘常见问题
    [ ] 增加串口协议常见问题
    [ ] 增加安全调试常见问题

## 2.13 推荐补齐顺序

第一阶段：先补所有项目都会用的基础能力

    1. ring_buffer
    2. crc
    3. limiter
    4. ramp
    5. low_pass_filter
    6. timeout_guard
    7. watchdog
    8. fault_manager
    9. test_record_template
    10. single_dm_motor_reference

第二阶段：补多执行器和底盘能力

    1. steer_wheel_kine 完善
    2. chassis_service
    3. swerve_module
    4. four_swerve_chassis_reference
    5. chassis debug 数据规范
    6. 舵轮调试流程
    7. 舵轮故障排查

第三阶段：补农业机器人通用能力

    1. differential_kine
    2. ackermann_kine
    3. wheel_odometry
    4. navigation_bridge
    5. gripper_service
    6. actuator_jam_detect
    7. power_monitor

第四阶段：补训练路线和经验库

    1. onboarding 文档
    2. 新人单电机训练任务
    3. 新人通信协议训练任务
    4. 新人底盘训练任务
    5. 常见问题索引
    6. 真实项目迁移案例

## 2.14 每个新增 SDK 的 Definition of Done

每个新增 SDK 合并前应满足：

    [ ] 有明确定位
    [ ] 有 public header
    [ ] public header 不暴露平台头文件
    [ ] 有状态码或错误返回
    [ ] 有 init 配置结构体
    [ ] 如需平台能力，device/infra 应通过 PortOps 接入
    [ ] 有 stop/disable 或等价安全接口
    [ ] 有参数单位说明
    [ ] 有最小使用示例
    [ ] 有测试记录
    [ ] 有常见错误说明
    [ ] README 或相关文档已同步更新

涉及执行器的 SDK 还必须满足：

    [ ] 有限幅
    [ ] 有 timeout
    [ ] 有安全停止
    [ ] 有低速测试流程
    [ ] 有急停或失能策略
    [ ] 不允许默认上电即大功率输出

涉及通信的 SDK 还必须满足：

    [ ] 有帧格式说明
    [ ] 有长度检查
    [ ] 有 CRC 或校验策略
    [ ] 有粘包/拆包处理说明
    [ ] 有心跳或数据过期判断
    [ ] 有异常帧处理策略

## 2.15 本计划的维护方式

本文件应随仓库一起维护

更新规则：

    新增 SDK 时，同步更新本文件对应地图
    新增 reference project 时，同步更新阶段计划
    修改架构边界时，同步更新第 1 部分
    从真实项目中发现通用问题时，同步补充第 2 部分工程经验库
    每个 release 前检查本文件与 README、通用开发流文档、实际目录是否一致

不建议：

    不建议在本文件中堆放临时想法
    不建议写无法验证的长期口号
    不建议把某个项目的特殊需求直接升级为协会标准
    不建议在没有参考工程前将复杂 SDK 标为稳定

推荐原则：

    先在真实项目验证
    再沉淀为 examples
    再抽象为 SDK
    最后进入标准文档
