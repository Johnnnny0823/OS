# myOSshell 測試紀錄

每項測試需執行並截圖，截圖檔名對應 `screenshots/` 目錄。

## 1. 編譯與啟動

```bash
make
./myOSshell
```

預期輸出：

```
myOSshell $
```

對應截圖：`01_start_shell.png`

## 2. 基本指令

| 指令 | 預期結果 |
|---|---|
| `ls` | 顯示目前目錄檔案 |
| `ls -l` | 長格式顯示檔案資訊 |
| `ps` | 顯示目前 process |
| `ps aux` | 顯示詳細 process 清單 |
| `ps axjf` | 顯示 process tree 格式 |

對應截圖：`02_ls_ps.png`、`03_ps_axjf_aux.png`

## 3. 指定 16 項指令

```bash
ps
ps axjf
ps aux
ls -l
ls -l | more
top
cal
whoami
date
pwd
mv old.txt new.txt
cp new.txt copy.txt
file copy.txt
cat copy.txt
rm copy.txt
vi myshell.c
```

說明：
- `top` 執行後按 `q` 離開，回到 `myOSshell $`。
- `vi` 進入編輯器後輸入 `:q` 離開，回到 `myOSshell $`。

對應截圖：`04_ls_l_more_pipe.png`、`07_file_cat_cp_mv_rm.png`、`08_vi_top_cal_date_pwd.png`

## 4. 其他建議指令

```bash
mkdir testdir
rmdir testdir
echo hello
clear
grep main myshell.c
```

## 5. 背景執行測試

```bash
sleep 10 &
ps
```

預期：
1. 立即回到 `myOSshell $`，不等待 `sleep 10` 結束
2. 顯示 `[background pid: <pid>]`
3. `ps` 可看到 `sleep` process
4. 等待數秒後背景 process 結束，不留下 zombie process（由 `SIGCHLD` handler 自動回收）

對應截圖：`05_background.png`

## 6. Pipe 測試

```bash
ls -l | more
ps aux | grep bash
ps aux | grep root | more
```

說明：本實作支援多重 pipe，三項測試皆可正常執行。

## 7. Redirection 測試

```bash
ls -l > output.txt
cat output.txt
echo hello >> output.txt
cat output.txt
cat < output.txt
```

預期：
1. `ls -l > output.txt` 建立檔案並寫入內容，畫面無輸出
2. `cat output.txt` 顯示寫入內容
3. `echo hello >> output.txt` 追加內容，不覆蓋原內容
4. `cat < output.txt` 從檔案讀取輸入並顯示

對應截圖：`06_redirection.png`

## 8. 內建指令測試

```bash
pwd
cd ..
pwd
cd /tmp
pwd
exit
```

預期：`cd` 確實改變 Shell 本身的工作目錄（而非僅 child process），`pwd` 反映正確路徑，
`exit` 正常結束 Shell。

## 9. 錯誤處理測試

| 輸入 | 預期錯誤訊息 |
|---|---|
| `abcdefg` | `myOSshell: command not found: abcdefg` |
| `cd nofolder` | `myOSshell: cd: no such file or directory` |
| `ls |` | `myOSshell: syntax error` |
| `| more` | `myOSshell: syntax error` |
| `cat < nofile.txt` | `myOSshell: nofile.txt: no such file or directory` |
| `ls >` | `myOSshell: missing output file` |
| `echo hello >>` | `myOSshell: missing output file` |

預期：Shell 在以上所有情況皆不崩潰，顯示錯誤訊息後回到 `myOSshell $` 提示字元。

對應截圖：`09_error_handling.png`

## 實測結果摘要（於 Ubuntu/WSL2 環境執行）

以上 9 大類測試已於 Ubuntu (WSL2) 環境下使用 `make` 編譯並逐項手動執行驗證，
包括基本指令、16 項指定指令、背景執行、單一與多重 pipe、`<`/`>`/`>>` 重新導向、
內建指令 `cd`/`pwd`/`exit`，以及全部 7 種錯誤輸入情境，Shell 均正常運作且未發生崩潰。
