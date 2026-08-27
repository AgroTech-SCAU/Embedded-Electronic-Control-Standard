# examples

`examples/` 只保存**用于理解开发标准和 SDK 用法的最小教学示例**

一个合适的 example 应尽量满足：

- 代码量小，可以在短时间内读完
- 只展示一个主要概念或开发模式
- 不绑定某一块真实控制板才能理解
- 不承担 SDK 正确性的长期硬件验证职责
- 可以被成员复制、改写或作为新模块起点

当前示例：

```text
examples/
└── module_design/
    └── portops_motor_device/
```

如果资产包含完整 CubeMX 工程、真实电机、板级 IO、硬件测试流程或 HIL 验证，应放入 [`../validation/`](../validation/) 而不是 `examples/`
