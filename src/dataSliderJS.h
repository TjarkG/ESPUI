const char JS_SLIDER[] PROGMEM = R"=====(
function rkmd_rangeSlider(selector){var self,slider_width,slider_offset,curnt,sliderDiscrete,range,slider;self=$(selector);slider_width=self.width();slider_offset=self.offset().left;sliderDiscrete=self;sliderDiscrete.each(function(i,v){curnt=$(this);curnt.append(sliderDiscrete_tmplt());range=curnt.find('input[type="range"]');slider=curnt.find(".slider");slider_fill=slider.find(".slider-fill");slider_handle=slider.find(".slider-handle");slider_label=slider.find(".slider-label");var range_val=parseInt(range.val());slider_fill.css("width",range_val+"%");slider_handle.css("left",range_val+"%");slider_label.find("span").text(range_val);});self.on("mousedown touchstart",".slider-handle",function(e){if(e.button===2){return false;}
var parents=$(this).parents(".rkmd-slider");var slider_width=parents.width();var slider_offset=parents.offset().left;var check_range=parents.find('input[type="range"]').is(":disabled");if(check_range===true){return false;}
$(this).addClass("is-active");var moveFu=function(e){var pageX=e.pageX||e.changedTouches[0].pageX;var slider_new_width=pageX-slider_offset;if(slider_new_width<=slider_width&&!(slider_new_width<"0")){slider_move(parents,slider_new_width,slider_width,true);}};var upFu=function(e){$(this).off(handlers);parents.find(".is-active").removeClass("is-active");};var handlers={mousemove:moveFu,touchmove:moveFu,mouseup:upFu,touchend:upFu,};$(document).on(handlers);});self.on("mousedown touchstart",".slider",function(e){if(e.button===2){return false;}
var parents=$(this).parents(".rkmd-slider");var slider_width=parents.width();var slider_offset=parents.offset().left;var check_range=parents.find('input[type="range"]').is(":disabled");if(check_range===true){return false;}
var slider_new_width=e.pageX-slider_offset;if(slider_new_width<=slider_width&&!(slider_new_width<"0")){slider_move(parents,slider_new_width,slider_width,true);}
var upFu=function(e){$(this).off(handlers);};var handlers={mouseup:upFu,touchend:upFu,};$(document).on(handlers);});}
function sliderDiscrete_tmplt(){var tmplt='<div class="slider">'+
'<div class="slider-fill"></div>'+
'<div class="slider-handle"><div class="slider-label"><span>0</span></div></div>'+
"</div>";return tmplt;}
function slider_move(parents,newW,sliderW,send){var slider_new_val=parseInt(Math.round((newW/sliderW)*100));var slider_fill=parents.find(".slider-fill");var slider_handle=parents.find(".slider-handle");var range=parents.find('input[type="range"]');range.next().html(newW);slider_fill.css("width",slider_new_val+"%");slider_handle.css({left:slider_new_val+"%",transition:"none","-webkit-transition":"none","-moz-transition":"none",});range.val(slider_new_val);if(parents.find(".slider-handle span").text()!=slider_new_val){parents.find(".slider-handle span").text(slider_new_val);var number=parents.attr("id").substring(2);if(send)websock.send("slvalue:"+slider_new_val+":"+number);}}
)=====";

const uint8_t JS_SLIDER_GZIP[881] PROGMEM = {
	31, 139, 8, 0, 3, 199, 199, 101, 2, 255, 237, 86, 77, 143, 218, 48, 16, 189, 243, 43, 88, 107, 187, 196, 93, 240,
	210, 61, 18, 204, 165, 85, 165, 30, 122, 106, 165, 86, 90, 173, 144, 73, 156, 141, 69, 112, 162, 216, 129, 182, 44,
	255, 189, 227, 143, 132, 36, 192, 106, 219, 83, 15, 61, 37, 246, 60, 143, 223, 204, 60, 123, 156, 84, 50, 210, 34,
	151, 195, 114, 189, 137, 151, 37, 147, 79, 252, 75, 38, 98, 94, 6, 138, 103, 60, 210, 121, 137, 247, 91, 86, 14, 97,
	148, 140, 149, 181, 44, 119, 34, 214, 105, 61, 200, 147, 68, 113, 61, 142, 170, 82, 106, 63, 247, 65, 168, 168, 228,
	154, 143, 173, 59, 63, 25, 26, 15, 244, 250, 232, 54, 108, 123, 163, 198, 74, 236, 111, 208, 88, 156, 107, 103, 114,
	255, 1, 38, 25, 79, 116, 216, 221, 199, 34, 122, 115, 132, 179, 40, 13, 18, 31, 93, 32, 198, 91, 188, 183, 28, 129,
	130, 78, 133, 194, 161, 29, 17, 86, 20, 92, 198, 65, 119, 241, 82, 111, 138, 12, 54, 195, 161, 141, 128, 58, 104,
	34, 0, 56, 18, 178, 168, 244, 131, 254, 89, 112, 138, 172, 21, 61, 142, 106, 198, 109, 32, 34, 110, 14, 53, 225, 36,
	34, 203, 168, 251, 239, 66, 38, 198, 114, 196, 165, 76, 198, 25, 63, 143, 116, 182, 35, 54, 99, 43, 126, 193, 169,
	53, 1, 210, 84, 207, 18, 93, 110, 89, 70, 11, 86, 42, 254, 73, 234, 192, 78, 17, 152, 50, 97, 182, 24, 146, 72, 169,
	0, 217, 74, 160, 113, 179, 238, 22, 189, 233, 19, 116, 64, 83, 142, 75, 56, 203, 192, 179, 82, 5, 147, 8, 19, 205,
	127, 248, 173, 13, 26, 135, 7, 28, 186, 242, 202, 0, 109, 242, 74, 241, 56, 223, 201, 161, 206, 171, 40, 85, 154,
	149, 224, 186, 31, 250, 184, 169, 41, 199, 123, 145, 4, 156, 172, 42, 173, 115, 73, 41, 189, 199, 123, 168, 30, 148,
	96, 152, 176, 76, 241, 240, 48, 48, 177, 67, 196, 92, 106, 85, 215, 157, 248, 49, 36, 202, 72, 126, 210, 84, 201,
	170, 188, 45, 73, 15, 108, 84, 217, 2, 120, 101, 214, 136, 174, 56, 13, 46, 74, 121, 180, 118, 199, 169, 65, 189,
	160, 31, 34, 128, 207, 44, 22, 138, 173, 50, 30, 3, 25, 8, 172, 237, 130, 82, 93, 86, 252, 36, 188, 58, 36, 22, 199,
	239, 51, 102, 202, 33, 212, 132, 65, 118, 182, 220, 7, 180, 201, 183, 252, 99, 69, 219, 57, 115, 57, 121, 226, 223,
	41, 39, 246, 251, 252, 12, 181, 76, 205, 62, 241, 87, 147, 120, 174, 30, 166, 143, 206, 212, 142, 89, 242, 93, 147,
	24, 48, 77, 58, 169, 48, 132, 251, 184, 57, 109, 167, 243, 230, 230, 234, 20, 129, 166, 8, 227, 189, 159, 54, 92, 3,
	159, 172, 113, 31, 218, 189, 123, 108, 54, 194, 195, 193, 242, 171, 138, 94, 132, 117, 90, 128, 90, 224, 84, 83,
	194, 121, 239, 148, 1, 145, 86, 166, 72, 201, 205, 222, 103, 82, 232, 54, 168, 125, 208, 189, 149, 168, 193, 206,
	92, 98, 199, 86, 169, 237, 9, 139, 168, 138, 153, 33, 229, 172, 112, 189, 184, 209, 33, 188, 14, 226, 60, 170, 54,
	64, 3, 27, 197, 31, 185, 189, 254, 24, 252, 215, 127, 47, 188, 19, 117, 122, 85, 255, 11, 250, 28, 252, 129, 60,
	207, 74, 237, 111, 132, 116, 24, 212, 155, 13, 207, 119, 53, 123, 3, 216, 127, 58, 154, 199, 98, 59, 140, 140, 240,
	41, 242, 74, 88, 140, 110, 7, 103, 230, 93, 147, 90, 204, 239, 192, 114, 9, 226, 175, 232, 197, 25, 147, 235, 70,
	139, 185, 233, 3, 139, 233, 252, 206, 126, 157, 179, 198, 37, 114, 127, 40, 244, 101, 182, 28, 79, 227, 233, 22, 2,
	42, 240, 205, 39, 31, 190, 144, 35, 255, 94, 57, 214, 167, 211, 246, 62, 51, 157, 146, 50, 175, 64, 138, 129, 89,
	122, 231, 151, 226, 183, 239, 166, 83, 220, 17, 185, 237, 215, 189, 91, 163, 219, 176, 91, 96, 223, 180, 207, 195,
	155, 174, 221, 244, 226, 215, 28, 10, 247, 248, 32, 210, 180, 76, 76, 82, 189, 201, 44, 227, 203, 221, 186, 27, 243,
	165, 150, 189, 55, 135, 116, 118, 138, 5, 209, 50, 169, 132, 73, 244, 12, 201, 92, 66, 171, 69, 147, 29, 95, 173,
	133, 158, 28, 77, 232, 104, 219, 228, 191, 206, 25, 14, 53, 113, 243, 182, 232, 110, 99, 207, 245, 75, 41, 26, 182,
	159, 9, 248, 138, 246, 150, 239, 95, 189, 182, 191, 175, 73, 188, 172, 54, 43, 120, 163, 213, 62, 152, 214, 37, 220,
	246, 112, 219, 16, 85, 173, 148, 46, 133, 124, 10, 238, 45, 69, 43, 35, 8, 93, 229, 209, 154, 152, 1, 188, 95, 50,
	240, 83, 241, 25, 186, 237, 103, 14, 166, 156, 103, 211, 144, 126, 3, 128, 124, 107, 46, 79, 11, 0, 0
};
