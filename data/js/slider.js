/* -----------------------------------------------------
  Material Design Sliders
  CodePen URL: https://codepen.io/rkchauhan/pen/xVGGpR
  By: Ravikumar Chauhan
-------------------------------------------------------- */
function rkmd_rangeSlider(selector) {
    let self, curnt, sliderDiscrete, range, slider;
    self = $(selector);
    sliderDiscrete = self;

    sliderDiscrete.each(function (i, v) {
        curnt = $(this);
        curnt.append(sliderDiscrete_tmplt());
        range = curnt.find('input[type="range"]');
        slider = curnt.find(".slider");
        slider_fill = slider.find(".slider-fill");
        slider_handle = slider.find(".slider-handle");
        slider_label = slider.find(".slider-label");

        const range_val = parseInt(range.val());
        slider_fill.css("width", range_val + "%");
        slider_handle.css("left", range_val + "%");
        slider_label.find("span").text(range_val);
    });

    self.on("mousedown touchstart", ".slider-handle", function (e) {
        if (e.button === 2) {
            return false;
        }
        const parents = $(this).parents(".slider");
        const slider_width = parents.width();
        const slider_offset = parents.offset().left;
        const check_range = parents.find('input[type="range"]').is(":disabled");
        if (check_range === true) {
            return false;
        }
        $(this).addClass("is-active");
        const moveFu = function (e) {
            const pageX = e.pageX || e.changedTouches[0].pageX;
            const slider_new_width = pageX - slider_offset;
            if (slider_new_width <= slider_width && !(slider_new_width < "0")) {
                slider_move(parents, slider_new_width, slider_width, true);
            }
        };
        const upFu = function (e) {
            $(this).off(handlers);
            parents.find(".is-active").removeClass("is-active");
        };

        var handlers = {
            mousemove: moveFu,
            touchmove: moveFu,
            mouseup: upFu,
            touchend: upFu,
        };
        $(document).on(handlers);
    });

    self.on("mousedown touchstart", ".slider", function (e) {
        if (e.button === 2) {
            return false;
        }

        const parents = $(this).parents(".slider");
        const slider_width = parents.width();
        const slider_offset = parents.offset().left;
        const check_range = parents.find('input[type="range"]').is(":disabled");

        if (check_range === true) {
            return false;
        }

        const slider_new_width = e.pageX - slider_offset;
        if (slider_new_width <= slider_width && !(slider_new_width < "0")) {
            slider_move(parents, slider_new_width, slider_width, true);
        }
        const upFu = function (e) {
            $(this).off(handlers);
        };

        var handlers = {
            mouseup: upFu,
            touchend: upFu,
        };
        $(document).on(handlers);
    });
}

function sliderDiscrete_tmplt() {
    return '<div class="slider">' +
        '<div class="slider-fill"></div>' +
        '<div class="slider-handle"><div class="slider-label"><span>0</span></div></div>' +
        "</div>";
}

function slider_move(parents, newW, sliderW, send) {
    const slider_new_val = parseInt(Math.round((newW / sliderW) * 100));

    const slider_fill = parents.find(".slider-fill");
    const slider_handle = parents.find(".slider-handle");
    const range = parents.find('input[type="range"]');
    range.next().html(newW); // update value

    slider_fill.css("width", slider_new_val + "%");
    slider_handle.css({
        left: slider_new_val + "%",
        transition: "none",
        "-webkit-transition": "none",
        "-moz-transition": "none",
    });

    range.val(slider_new_val);
    if (parents.find(".slider-handle span").text() != slider_new_val) {
        parents.find(".slider-handle span").text(slider_new_val);
        const number = parents.attr("id").substring(2);
        if (send) websock.send("slvalue:" + slider_new_val + ":" + number);
    }
}
