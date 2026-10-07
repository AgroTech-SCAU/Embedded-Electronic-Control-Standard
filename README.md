# Embedded-Electronic-Control-Standard

AgroTech 协会公共 MCU SDK，供成员项目按需引用，Longinus 所需模块的真机验证已由负责人确认通过

正式代码位于 `sdks/infra` `sdks/domain` `sdks/device`，项目负责 app service assemble platform 与机械安装配置

| 入口 | 用途 |
|---|---|
| [SDK 状态](docs/module-status.md) | 判断哪些模块可固定使用 |
| [SDK 接入](sdks/README.md) | 源码选择 初始化与平台注入 |
| [架构](docs/architecture.md) | 六层项目架构与公共 SDK 边界 |
| [验证](validation/README.md) | 主机命令 板级接入和硬件结果来源 |
| [发布规则](docs/release-policy.md) | 兼容性 变更和发布 Gate |
| [实施记录](docs/plan.md) | 五步任务结果与剩余限制 |
| [协作指南](.github/CONTRIBUTING.md) | Issue Branch PR 和负责人合并 |

## 快速引用

成员项目使用 submodule 固定负责人验证过的 Tag 或 commit，不跟随远端分支自动更新

```sh
git submodule add https://github.com/AgroTech-SCAU/Embedded-Electronic-Control-Standard.git external/Embedded-Electronic-Control-Standard
cd external/Embedded-Electronic-Control-Standard
git fetch origin --tags
git switch --detach <verified-tag-or-commit>
cd ../..
git add .gitmodules external/Embedded-Electronic-Control-Standard
git commit -m "chore(submodule): pin embedded SDK"
```

克隆或同步成员项目后执行

```sh
git submodule update --init --recursive
```

将选定模块的正式源文件加入构建，include 根目录使用 `sdks/device` `sdks/infra` `sdks/domain`，具体依赖见模块头文件和验证清单

由项目 service/assemble 创建 PortOps 和实例存储，再初始化 SDK，PortOps 与存储的生命周期覆盖 SDK 使用周期

## 开发与验证

```sh
sh validation/build.sh /tmp/embedded-sdk-validation
bash setup-scripts/setup-git.sh
```

Windows 可执行 setup-scripts/setup-git.ps1 配置本地 Hook

主机验证不生成 MCU 固件，validation 中的板级目录是集成资产，完整 HAL CMSIS 启动文件与链接脚本由成员项目提供

当前提供 DM DJI FS-iA10B 验证入口，负责人已确认步骤 1～3 真机通过，详细硬件日志尚未随包提供，证据边界见 [验证记录](validation/hardware-evidence.md)

涉及执行机构时，项目必须处理 stop 反馈超时 错误返回和人工恢复，SDK 软件失能不等于物理断电

仓库保护配置由管理员按 [Ruleset 指南](.github/rulesets/README.md) 导入，本地 Hook 不替代远端保护
