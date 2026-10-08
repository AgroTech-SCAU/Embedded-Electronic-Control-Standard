# HFSM

轻量级层级有限状态机库

适合需要父子状态 Entry Exit Action 事件队列和显式状态切换的 MCU 任务逻辑

## 最小流程

```c
Hfsm fsm;

hfsm_init(&fsm, &context);
HfsmState* idle = hfsm_add_state(&fsm, "Idle");
HfsmState* run  = hfsm_add_state(&fsm, "Run");

hfsm_set_handle(idle, idle_handle);
hfsm_set_handle(run, run_handle);
hfsm_set_initial(&fsm, idle);
hfsm_start(&fsm);
```

事件处理函数返回

```c
hfsm_ignore();
hfsm_handled();
hfsm_transition(run);
```

主循环投递并处理事件

```c
hfsm_post(&fsm, EVENT_START, NULL);
hfsm_process_all(&fsm);
```

## 层级状态

使用 `hfsm_add_substate` 或 `hfsm_set_parent` 建立父子关系

未被子状态处理的事件可以继续交给父状态

`entry` `exit` `action` 分别处理进入 退出和周期行为

## 生命周期

常用接口

```text
hfsm_start
hfsm_pause
hfsm_go_on
hfsm_reset
hfsm_clear
```

当前状态可通过 `hfsm_state` 获取

## 使用约束

- 状态对象和 user_data 生命周期必须覆盖状态机运行周期
- action 应保持非阻塞
- 库本身不负责线程安全
- 中断中优先只投递轻量事件
- 复杂业务应在项目 app/service 中封装 不把业务逻辑写进 HFSM 库

完整 API 和配置项以 `hfsm.h` `hfsm_core.h` `hfsm_config.h` 为准
