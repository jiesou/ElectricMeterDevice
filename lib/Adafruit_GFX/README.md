# Adafruit GFX（本地裁剪副本）

上游：https://github.com/adafruit/Adafruit_GFX
版本：1.12.6（取自 PlatformIO 缓存的 `adafruit/Adafruit GFX Library`）
许可：BSD，见 `license.txt`

## 改了什么

只保留画布部分：`Adafruit_GFX.{h,cpp}`、`gfxfont.h`、`glcdfont.c`。

- 删掉 `Adafruit_GrayOLED.*`、`Adafruit_SPITFT.*`（灰度 OLED / SPI TFT，本工程不用）
- 删掉 `Adafruit_GFX.h` 里 `<Adafruit_I2CDevice.h>`、`<Adafruit_SPIDevice.h>` 两行 include

这两处都是为了不让 `Wire` + 整套 I2C 驱动（约 26 KB）被链接进来。工程要接 I2C 外设时，
把这两行和上面两个文件补回来即可。

## 升级

从上游拷新的同名文件覆盖，再重复上面两步裁剪。
