var oled_gd = require('./build/Release/oled_gd');

var oled = oled_gd.oled();
oled.demo();

oled.blitString("hello", 12, 12);
oled.display();
oled.showImage(image, 0, 0);

oled.display();

var img2 = oled_gd.create(128, 64);
var black = img2.colorAllocate(0, 0, 0);
var white = img2.colorAllocate(255, 255, 255);

img2.line(0, 0, 128, 64, white);
img2.line(0, 64, 128, 0, white);
img2.stringBIF(white, "large", 0, 0, "hello world");
img2.stringBIF(white, "medium", 0, 16, "hello world");
img2.stringBIF(white, "small", 0, 32, "hello world");
img2.stringBIF(white, "giant", 0, 48, "hello world");
oled.showImage(img2, 0, 0);
oled.display();
                                                      
