# 临时迁移固件（-tmp-8bit-read-4bit-write）

面向 **HG5382A / HG5585F-CT / HG5585F-CU（Winbond W29N02KVSIAF）** 的一次性
U-Boot：**按原厂 8-bit 排布读闪存，把整片 UBI 以 4-bit 重写**，之后交回官方
4-bit 引导栈。目标是让这三款机型**不用 TTL** 也能从原厂固件直接刷到官方版。

---

## 一、为什么是「读 8 写 4」

写进闪存的 ECC 排布由**执行写入的那个软件**决定，不由镜像文件决定：

| 软件 | Winbond 用几 bit | 谁写闪存 |
|---|---|---|
| 原厂内核（stock2ubi 跑在上面） | 8 | 它写 |
| 官方 BL2（preloader） | 4 | — |
| 官方 U-Boot | 4 | 它写 |

所以「用 8-bit 的 U-Boot 去刷官方 4-bit 的 U-Boot」拿不到 4-bit 数据 ——
落盘的还是 8-bit，重启后官方 BL2 照旧读不出来，只是把砖挪到了刷完之后。

本固件因此拆成两段行为：

1. **读**：控制器按 8 bits/512B 运行 → 原厂内核写的那片 UBI 可以正常挂载；
2. **写**：点「重建 UBI」时**先切到 4 bits/512B**，再擦除、重建、写入 →
   落盘就是官方 4-bit 排布。

切换只在「重建 UBI」这一处发生，因为那也是唯一一个「旧数据全部作废」的时刻。
在切换之前，网页会**拒绝**写 FIP / 写系统 / 写板级数据，避免出现半 8 半 4 的闪存。

---

## 二、产物

每个机型三个文件（官方版本的文件名不变，仍是不带标记的 `*-preloader.bin` 等）：

```
an7581-fiberhome-hg5382a-preloader-tmp-8bit-read-4bit-write.bin
an7581-fiberhome-hg5382a-firstblock-tmp-8bit-read-4bit-write.bin
an7581-fiberhome-hg5382a-bl31-u-boot-tmp-8bit-read-4bit-write.fip
```

`hg5585f-ct`、`hg5585f-cu` 同理。

- `*-preloader-...bin` / `*-firstblock-...bin`：**BL2 用 8 bits/512B** 的版本
  （官方版是 4）。与迁移版 U-Boot 配套，读原厂闪存自洽。
- `*-bl31-u-boot-...fip`：迁移版 U-Boot（8-bit 读 / 4-bit 写）。

重新构建：

```sh
export CROSS_COMPILE=aarch64-linux-gnu-
export ARM32_CROSS_COMPILE=arm-none-eabi-
export MBEDTLS_DIR=/path/to/mbedtls-3.4.1   # 必须 3.4.x，3.6 的默认配置与 TF-A 不符
export BUILD_JOBS=$(nproc)

BRIDGE_ECC=1 BRIDGE_TBB=0 ./scripts/build-an758x.sh hg5382a
```

`BRIDGE_TBB=0` 让迁移版 BL2 跳过镜像验签（见第八节）；如果手上有官方
AIROHA_SIGN_KEY，去掉这个变量即可正常签名。

产物在 `output/<机型>-tmp-8bit-read-4bit-write/`。

---

## 三、路线 A：不用 TTL（推荐先验证这条）

1. 在原厂系统里跑 stock2ubi，**先做完整备份**。
2. 上传**迁移版**的 `*-preloader-...bin`（或 `*-firstblock-...bin`）和
   **迁移版** `*-bl31-u-boot-...fip`，刷入并重启。
3. 串口会打印 `MIGRATION build (8-bit read / 4-bit write)`，进入 U-Boot。
   浏览器打开 `http://192.168.0.1/`，页面顶部有红色提示条。
4. 先把**板级数据卷**下载备份（这一步是 8-bit 读，只有本固件能做）。
5. 点 **重建 UBI**。串口会打印 `moving the flash to the released 4-bit ECC layout`，
   随后清空并重建 UBI。
   **注意：重建会擦掉整个 UBI 分区，之前（8-bit 阶段）刷进去的 fip 一并丢失**，
   所以下面第 7 步必须重刷一次 fip。漏掉就会卡在
   `ERROR: BL2: Failed to load image id 3 (-22)` + XMODEM。
6. 上传并写入**官方版** `*-preloader.bin` / `*-firstblock.bin`（4-bit）。
7. 上传并写入**官方版** `*-bl31-u-boot.fip`（4-bit）。
8. 恢复板级数据卷。
9. 上传 sysupgrade → 刷写系统 → 重启。

第 6～9 步全部由已经切到 4-bit 的控制器完成，重启后官方 BL2（4-bit）读 4-bit
闪存，整条链一致。

> BL2 分区（第一个 eraseblock）由 BootROM 按主数据区读取，OOB 里的校验码不参与，
> 所以第 6 步在 4-bit 下写官方 preloader 是安全的。

---

## 四、路线 B：已经有官方 4-bit preloader（TTL + XMODEM）

如果机器上已经是官方 preloader、且原厂 UBI 仍是 8-bit，BL2 读不到 fip 卷会停在
`Press x to load BL31 + U-Boot FIP via XMODEM`：

1. 按 `x`，用 XMODEM 发送**迁移版** `*.fip`（不要发官方 fip）。
2. 起来之后从路线 A 的第 4 步继续。

这条路不写 BL2，所以第 6 步改成「上传官方 preloader 并由本固件写入」，
其余一致。

---

## 五、串口等价命令

```
nandecc          # 打印当前 on-flash ECC（迁移前应为 8，迁移后为 4）
nandecc 4        # 手动切换（会自动 ubi detach），之后必须先重建 UBI 再写任何东西
```

手动重建 UBI 的等价流程就是网页那个按钮，命令行没有等价的一步式命令，
建议仍用网页操作。

---

## 六、关键警告

- **重建 UBI 之后，不要再刷本固件（迁移版）**。它写进闪存的仍是 8-bit 排布，
  而那时 BL2 已经是 4-bit，重启后必然变砖，只能 TTL 救砖。网页红色提示条和
  重建完成的控制台输出都会重复这句话。
- 迁移前直接写 FIP / 系统 / 板级数据会被拒绝并提示先重建 UBI。
- 重建 UBI 之后，「重启」「启动系统」会被拒绝，直到官方 preloader 和
  官方 fip 都写入成功（会话内计数，重启后失效）。这是为了防「重建后忘了
  重刷 fip 就重启」这个必砖路径。
- 「恢复整片 Flash」不走这个开关：它按当前强度写入，恢复到原厂备份时用 8-bit
  （迁移前），恢复到官方栈的整片镜像时先 `nandecc 4`。

---

## 六点五、刷到别的颗粒上会怎样

迁移版**只认一颗颗粒**：完整 ID `ef da 10 95 06`（W29N02KVSIAF）。
不匹配时（W29N02KVSIAE 是 `…07`、W29N02GV 是 `90 95 04`、东芝、
Spansion、Micron 等），运行时标志 `bridge_active` 保持为假，结果是：

- ECC 强度取配置表里的原值（东芝 8、其余 4），不会被强制改；
- 不拦 FIP / 系统 / 板级数据的写入；
- 重建 UBI 不会被切到 4-bit（这一点最关键：东芝机是 8-bit，
  切过去会砖）；
- `/api/device` 里 `bridge` 为 false、`temp_build` 仍为 true。

也就是说，除目标颗粒外，这份固件对闪存的行为与官方版**完全一致**。

不过页面仍会显示一条提示（因为这份固件本身是临时产物）：说明它是专为
W29N02KVSIAF 准备的临时迁移固件、不随官方发布更新、当前颗粒不需要它，
建议刷回官方 preloader 与 bl31-u-boot.fip 长期使用。

迁移版 **preloader** 同理：`AIROHA_NAND_ECC_BRIDGE` 只改了表中
W29N02KVSIAF 那一行，其它颗粒与官方 preloader 等价；串口会多打一行
`Migration preloader: built for W29N02KVSIAF, <型号> keeps stock ECC`。

**已迁移过的机器**（同一颗 Winbond，但闪存已经是 4-bit）：迁移版 U-Boot
会先按 8-bit 启动，挂 UBI 失败时自动切回 4-bit 重试一次并打印
`this board is probably already on the released layout`，不需要手动
`nandecc 4`。

## 七、首次实机验证清单

1. 迁移版 BL2 启动时串口打印
   `BL2: migration build, image authentication disabled` 和
   `Parallel NAND: W29N02KVSIAF ... ECC8/512`。
2. 迁移版 U-Boot 里 `ubi part ubi` **能成功** —— 这一条同时验证了本方案唯一的
   关键假设：本驱动的 8-bit 排布（每 512B 扇区 8B FDM + 28B 备用区）与原厂内核
   写出来的一致。
3. 重建 UBI 之后 `nandecc` 显示 4。
4. 重启后官方 BL2 能找到 fip 卷（不再进 XMODEM）。

---

## 八、风险与未覆盖

- **签名**：官方发布是**签名**的（CI 用 `secrets.AIROHA_SIGN_KEY`），
  BL2 默认走 `load_auth_image()` 校验证书链，未签名 FIP 会被拒绝并停在
  XMODEM。这批迁移固件因为拿不到那把私钥，改成在 BL2 里
  `dyn_disable_auth()`（`BRIDGE_TBB=0`，串口会打印
  `BL2: migration build, image authentication disabled`），因此**不需要签**。
  迁移完成后刷入的官方 BL2/FIP 仍是已签名的官方产物，最终状态不变。
  注意：迁移版 BL2 不校验任何镜像，只用于这一次迁移。
- **8-bit 排布一致性**：见验证清单第 2 条，这是唯一的硬件假设，没实机验证过。
- **整片恢复**：见第六节。
- **回滚**：回到原厂需要整片备份 + 对应强度的写入；原厂镜像只能在 8-bit 下写
  （`nandecc 8`），且回滚前不要把 UBI 重建到 4-bit。

---

## 九、为编过这棵树顺带修的两处（与本功能无关）

- `drivers/mtd/nand/raw/nand_esmt.c`：把 `gigadevice_nand_manuf_ops`（ops 类型）
  赋给 `manufacturer.desc`（desc 类型）是类型错误，GCC 14+ 直接报错；
  改为指向新增的 `gigadevice_nand_manuf_desc`。
- `tf-a/drivers/auth/mbedtls/mbedtls_common.mk`：`hash_info.c` 在部分 mbedtls 3
  源码树里不存在，改为仅在文件存在时加入源码列表。
