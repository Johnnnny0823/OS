# myOSshell

## 專案簡介
myOSshell 是一個以 C 語言實作的微型 Linux Shell，透過 `fork()` / `execvp()` / `waitpid()`
建立並管理子處理元，並自行解析指令、支援背景執行 (`&`)、管線 (`|`) 與
輸入／輸出重新導向 (`<`、`>`、`>>`)，藉此呈現作業系統中 process 管理與
file descriptor 操作的核心概念。

## 開發環境
- Ubuntu Linux (建議使用 Ubuntu 22.04 或 WSL2 + Ubuntu)
- GCC

## 編譯方式

```bash
make
```

或不使用 Makefile：

```bash
gcc myshell.c parser.c executor.c -o myOSshell
```

## 執行方式

```bash
./myOSshell
```

## 程式架構

```text
myshell.h    -- 共用資料結構 (Command, ParsedLine) 與函式宣告
myshell.c    -- main() 主迴圈、提示字元、讀取輸入、SIGCHLD 處理
parser.c     -- 指令字串解析（pipe / redirection / background）
executor.c   -- 內建指令、fork/execvp/waitpid、管線與重新導向執行
```

## 支援功能
- `fork()` / `execvp()` / `waitpid()` 建立並管理子處理元
- 背景執行 `&`，並顯示背景 process PID
- 使用 `SIGCHLD` + `waitpid(WNOHANG)` 回收已結束的背景 process，避免 zombie
- 管線 `|`（支援多重 pipe，例如 `ps aux | grep root | more`）
- 輸入重新導向 `<`
- 輸出重新導向 `>`（覆蓋）與 `>>`（追加）
- 內建指令：`cd`（使用 `chdir()`）、`pwd`（使用 `getcwd()`）、`exit`
- 錯誤處理：空白輸入、不存在的指令、`cd` 到不存在目錄、pipe/redirection 語法錯誤、
  `fork()`/`execvp()`/`pipe()`/`open()` 失敗時皆顯示錯誤訊息並回到提示字元，不會讓
  Shell 崩潰

## 支援指令清單
依老師指定的 16 項指令：

```
ps, ps axjf, ps aux, ls -l, ls -l | more, top, cal, whoami,
date, pwd, mv, cp, file, cat, rm, vi
```

以上指令皆透過 Shell 自行 `fork()` + `execvp()` 執行外部程式，並非寫死輸出。

另外支援：`mkdir`、`rmdir`、`echo`、`clear`、`grep`、`more`。

## 測試指令
詳見 [test_cases.md](test_cases.md)。

## 已知限制
1. `top`、`vi` 為全螢幕互動式程式：`top` 需按 `q` 離開，`vi` 需輸入 `:q` 或 `:wq` 離開，
   離開後會自動回到 `myOSshell $` 提示字元。
2. 目前未實作指令歷史（history）、Tab 自動補全、別名（alias）等進階 shell 功能。
3. 多重 pipe（如 `ps aux | grep root | more`）已支援，最多可串接 `MAX_COMMANDS`（8）個指令。
4. 本程式依賴 UNIX 系統呼叫（`fork`、`execvp`、`pipe`、`waitpid` 等），僅能在
   Linux / UNIX 類環境（包含 WSL2 + Ubuntu）下編譯執行，無法在 Windows 原生環境執行。

## 截圖說明
截圖存放於 `screenshots/` 目錄，對應如下：

| 檔名 | 內容 |
|---|---|
| 01_start_shell.png | Shell 啟動畫面 |
| 02_ls_ps.png | `ls`、`ps` 基本指令 |
| 03_ps_axjf_aux.png | `ps axjf`、`ps aux` |
| 04_ls_l_more_pipe.png | `ls -l | more` 管線 |
| 05_background.png | `sleep 10 &` 背景執行與 PID |
| 06_redirection.png | `>`、`>>`、`<` 重新導向 |
| 07_file_cat_cp_mv_rm.png | `file`、`cat`、`cp`、`mv`、`rm` |
| 08_vi_top_cal_date_pwd.png | `vi`、`top`、`cal`、`date`、`pwd` |
| 09_error_handling.png | 各類錯誤輸入處理 |
