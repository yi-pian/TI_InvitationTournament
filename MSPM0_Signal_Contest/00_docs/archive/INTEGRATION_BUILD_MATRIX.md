# Integration Build Matrix

最终权威矩阵见 `FINAL_INTEGRATION_BUILD_MATRIX.md`。本文件保留阶段关系：Round 1 pre-backend 基线不可删除；Q31 migration 与 Final Sprint 产物另目录保存，互不覆盖。

| Stage | Targets | SysConfig | Compile | Link | Map/Out | Board |
|---|---:|---|---|---|---|---|
| PRE-BACKEND SYSTEM BASELINE | 8 | 8/8 PASS | 8/8 PASS | 8/8 PASS | 8/8 | PENDING_BOARD |
| Round 1 CMSIS Q31 migration | 4 | 4/4 PASS | 4/4 PASS | 4/4 PASS | 4/4 | PENDING_BOARD |
| Final applications | 4 | 4/4 PASS | 4/4 PASS | 4/4 PASS | 4/4 | PENDING_BOARD |
| Signal Analyzer profiles | 5 | 5/5 PASS | 5/5 PASS | 5/5 PASS | 5/5 | PENDING_BOARD |
| Contest Template profiles | 4 | 4/4 PASS | 4/4 PASS | 4/4 PASS | 4/4 | PENDING_BOARD |

状态规则：没有板上证据时最高为 BUILD/PC_VERIFIED，禁止写 BOARD_VERIFIED 或 CONTEST_VERIFIED。N=2048 Frequency C 的真实链接失败和 N=4096 不支持状态保留在最终矩阵中。
