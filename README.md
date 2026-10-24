# AN758x U-Boot

U-Boot for Airoha AN7581/AN7583 with a bilingual Web recovery interface.

面向 Airoha AN7581/AN7583 的 U-Boot，集成中英文 Web 恢复页面。

Hold Reset about one second after power-on to enter Web recovery.

上电约一秒后按住 Reset，即可进入 Web 恢复页面。

## Supported devices / 支持机型

| Target / 构建目标 | Device / 设备 | SoC | Storage / 存储 |
| --- | --- | --- | --- |
| `hg5382a` | FiberHome HG5382A | AN7581 | Parallel NAND |
| `hg5585f-ct` | FiberHome HG5585F CT | AN7581 | Parallel NAND |
| `hg5585f-cu` | FiberHome HG5585F CU | AN7581 | Parallel NAND |
| `hm2004-du` | H3C HM2004-DU | AN7581 | SPI NAND |
| `xg2010g` | Gemtek XG2010G | AN7581 | SPI NAND |
| `xr1710g` | Gemtek XR1710G | AN7581 | SPI NAND |
| `zn504xg-d` | ZNXT ZN504XG-D | AN7581 | SPI NAND |
| `zn515xg-d` | ZNXT ZN515XG-D | AN7581 | SPI NAND |
| `ung00a` | UnionMan UNG00A | AN7581 | SPI NAND |
| `xg-040g-md` | Nokia XG-040G-MD | AN7581 | SPI NAND |
| `xg-040g-tf` | Nokia XG-040G-TF | AN7581 | SPI NAND |
| `xg-040g-mf` | Nokia XG-040G-MF | AN7583 | SPI NAND |

## Winbond W29N02KVSIAF / 华邦闪存机型

Some HG5382A / HG5585F units ship Winbond `W29N02KVSIAF` instead of Toshiba
`TC58NVG1S3HTAI0`. The vendor firmware programs the whole chip with 8-bit BCH
ECC while this project uses 4-bit, so that hardware batch bricks when flashed
through the one-shot Stock2UBI flow. A temporary migration build bridges the
two.

部分 HG5382A / HG5585F 用的是 Winbond `W29N02KVSIAF`（不是东芝
`TC58NVG1S3HTAI0`）。原厂固件按 8-bit BCH 写整片闪存，本工程是 4-bit，
两者错位会让这批机器在 Stock2UBI 一键刷写后变砖。为此提供一份临时迁移固件。

```sh
BRIDGE_ECC=1 BRIDGE_TBB=0 ./scripts/build-an758x.sh hg5382a
```

Artifacts land in `output/<target>-tmp-8bit-read-4bit-write/`. The build reads
the flash at the vendor 8-bit codeword and switches the controller to 4-bit
when UBI is rebuilt, so the board ends up on the normal 4-bit stack.

产物在 `output/<target>-tmp-8bit-read-4bit-write/`：读写按原厂 8-bit 布局，
点「重建 UBI」时切到 4-bit，最后回到常规的 4-bit 引导栈。

1. Flash the migration `*-preloader-tmp-8bit-read-4bit-write.bin` (or
   `*-firstblock-*`) and `*-bl31-u-boot-tmp-8bit-read-4bit-write.fip` with
   Stock2UBI or by hand.<br>
   用 Stock2UBI（或手动）写入迁移版 `*-preloader-*` / `*-firstblock-*` 和
   `*-bl31-u-boot-*`.fip。
2. Open the Web recovery page and select **Rebuild UBI**. The flash switches to
   the 4-bit layout and the UBI partition, including the FIP volume, is erased.<br>
   打开网页选 **重建 UBI**；此时闪存切到 4-bit，整个 UBI 分区（含 fip 卷）被清空。
3. **Re-flash the official `*-preloader.bin` and `*-bl31-u-boot.fip` after the
   rebuild.** Skipping this leaves BL2 without a FIP and the board stops at
   XMODEM. The migration U-Boot refuses to reboot until both are written.<br>
   **重建之后必须重新刷官方 `*-preloader.bin` 和 `*-bl31-u-boot.fip`**。
   漏刷会停在 BL2 的 XMODEM 提示；迁移版 U-Boot 在两步完成前会拒绝重启。
4. Restore board data, upload the sysupgrade image, boot the system.<br>
   恢复板级数据、上传 sysupgrade、启动系统。

The migration build disables image authentication (`BRIDGE_TBB=0`) so it needs
no release signing key; the official 4-bit stack flashed afterwards keeps its
own signed chain. It only takes effect on `W29N02KVSIAF` — on any other part it
behaves exactly like a stock build and shows a reminder that it is a temporary
image. See [NAND-ECC-临时迁移固件说明.md](NAND-ECC-临时迁移固件说明.md) for the
long version.

迁移版关闭了镜像验签（`BRIDGE_TBB=0`），不需要发布私钥；之后刷入的官方
4-bit 栈仍是签名产物。它只在 `W29N02KVSIAF` 上生效，其他颗粒行为与官方构建
一致，页面上会提示这是临时固件。详见
[NAND-ECC-临时迁移固件说明.md](NAND-ECC-临时迁移固件说明.md)。

## Build / 构建

```sh
export CROSS_COMPILE=/path/to/aarch64-linux-musl-
export ARM32_CROSS_COMPILE=/path/to/arm-none-eabi-
export MBEDTLS_DIR=/path/to/mbedtls-3.4.1
export BUILD_JOBS=$(nproc)

./scripts/build-an758x.sh hg5585f-ct
```
Artifacts are written to `output/<target>/` / 产物位于 `output/<target>/`

## Installation / 刷入

### Automatic / 自动刷机

Run [AN758x-Stock2UBI](https://github.com/pbs05/an758x-stock2ubi) from the
stock system. Back up the needed partitions, then upload `*-preloader.bin` or
`*-firstblock.bin` together with
`*-bl31-u-boot.fip`. The tool writes the recovery bootloader and reboots.<br>
在原厂系统中运行 [AN758x-Stock2UBI](https://github.com/pbs05/an758x-stock2ubi)，
备份需要保留的分区，然后上传 `*-preloader.bin` 或 `*-firstblock.bin`，以及
`*-bl31-u-boot.fip`。工具写入恢复引导后自动重启。

### Manual / 手动刷机

1. Back up every MTD partition and UBI volume.<br>
   备份全部 MTD 分区和 UBI 卷。
2. Write one BL2 image at the beginning of NAND:<br>
   在 NAND 开头写入以下一种 BL2 镜像：

   - Write `*-firstblock.bin` from NAND offset `0x0`.<br>
     从 NAND `0x0` 偏移写入 `*-firstblock.bin`。
   - Leave the first `0x800` bytes erased and write `*-preloader.bin` from
     NAND offset `0x800`.<br>
     保持前 `0x800` 字节为擦除态，从 NAND `0x800` 偏移写入
     `*-preloader.bin`。
3. Boot the device. When BL2 requests a FIP, press `x` and send
   `*-bl31-u-boot.fip` through XMODEM.<br>
   启动设备；BL2 请求 FIP 时按 `x`，通过 XMODEM 发送
   `*-bl31-u-boot.fip`，临时进入 U-Boot。

With either method, connect Ethernet and open `http://192.168.0.1/` (the first
boot may take about one minute). For the first installation, select
**Rebuild UBI / 重建 UBI**, then write `*-bl31-u-boot.fip`, restore the board-data
volumes, upload the sysupgrade image, and select **Boot system / 启动系统**.<br>
通过以上任一方法进入 U-Boot 后，连接网线并打开 `http://192.168.0.1/`（首次启动
可能需要约一分钟）。首次安装选择 **重建 UBI**，随后写入 `*-bl31-u-boot.fip`、
恢复板级数据卷、上传 sysupgrade 镜像，最后选择 **启动系统**。

Subsequent upgrades use **Install system / 刷写系统** directly.

后续升级直接使用 **刷写系统**。

## Upstream fix / 上游修复

The migration image is a workaround. The permanent fix is to make the whole
chain agree on one ECC strength: because the vendor kernel writes 8-bit,
setting the Winbond entry back to 8 in both
`tf-a/plat/ecnt/common/drivers/flash/parallel_nand_flash_table.c`
(`soc_ecc_ability`) and `drivers/mtd/nand/raw/nand_ids.c`
(`NAND_ECC_INFO`) makes BL2 and U-Boot match the stock-written flash, and the
temporary build is no longer needed. The remaining unknown is the OpenWrt MTD
driver, which has to use the same strength.

迁移固件只是权宜之计。真正的修法是让整条链统一到同一种 ECC 强度：原厂内核
是按 8-bit 写的，所以把 Winbond 这一项在
`tf-a/plat/ecnt/common/drivers/flash/parallel_nand_flash_table.c` 的
`soc_ecc_ability` 和 `drivers/mtd/nand/raw/nand_ids.c` 的
`NAND_ECC_INFO` 里都改回 8，BL2 与 U-Boot 就与原厂写入的闪存一致，临时
固件也就不再需要。唯一还没确认的是 OpenWrt 的 MTD 驱动是否用同一强度。

The CI in this fork builds only the three migration targets
(`hg5382a`, `hg5585f-ct`, `hg5585f-cu`); upstream builds all twelve.

本 fork 的 CI 只构建这三款迁移固件；上游构建全部十二个机型。

## TF-A sources / TF-A 来源

- [Ansuel/atf-airoha](https://github.com/Ansuel/atf-airoha)
- [mkshevetskiy/atf-airoha](https://github.com/mkshevetskiy/atf-airoha)
- [Yuzhii0718/atf-airoha](https://github.com/Yuzhii0718/atf-airoha)
