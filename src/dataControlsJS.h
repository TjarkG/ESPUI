const char JS_CONTROLS[] PROGMEM = R"=====(
const UI_INITIAL_GUI=200;const UI_RELOAD=201;const UPDATE_OFFSET=100;const UI_EXTEND_GUI=210;const UI_TITEL=0;const UI_PAD=1;const UPDATE_PAD=101;const UI_CPAD=2;const UPDATE_CPAD=102;const UI_BUTTON=3;const UPDATE_BUTTON=103;const UI_LABEL=4;const UPDATE_LABEL=104;const UI_SWITCHER=5;const UPDATE_SWITCHER=105;const UI_SLIDER=6;const UPDATE_SLIDER=106;const UI_NUMBER=7;const UPDATE_NUMBER=107;const UI_TEXT_INPUT=8;const UPDATE_TEXT_INPUT=108;const UI_GRAPH=9;const ADD_GRAPH_POINT=10;const CLEAR_GRAPH=109;const UI_TAB=11;const UPDATE_TAB=111;const UI_SELECT=12;const UPDATE_SELECT=112;const UI_OPTION=13;const UPDATE_OPTION=113;const UI_MIN=14;const UPDATE_MIN=114;const UI_MAX=15;const UPDATE_MAX=115;const UI_STEP=16;const UPDATE_STEP=116;const UI_GAUGE=17;const UPDATE_GAUGE=117;const UI_ACCEL=18;const UPDATE_ACCEL=118;const UI_SEPARATOR=19;const UPDATE_SEPARATOR=119;const UI_TIME=20;const UPDATE_TIME=120;const UI_FILEDISPLAY=21;const UPDATE_FILEDISPLAY=121;const UI_FRAGMENT=98;const UP=0;const DOWN=1;const LEFT=2;const RIGHT=3;const CENTER=4;const C_TURQUOISE=0;const C_EMERALD=1;const C_PETERRIVER=2;const C_WETASPHALT=3;const C_SUNFLOWER=4;const C_CARROT=5;const C_ALIZARIN=6;const C_DARK=7;const C_NONE=255;var controlAssemblyArray=new Object();var FragmentAssemblyTimer=new Array();var graphData=new Array();var hasAccel=false;var sliderContinuous=false;function colorClass(colorId){colorId=Number(colorId);switch(colorId){case C_TURQUOISE:return"turquoise";case C_EMERALD:return"emerald";case C_PETERRIVER:return"peterriver";case C_WETASPHALT:return"wetasphalt";case C_SUNFLOWER:return"sunflower";case C_CARROT:return"carrot";case C_ALIZARIN:return"alizarin";case C_DARK:case C_NONE:return"dark";default:return"";}}
var websock;var websockConnected=false;var WebSocketTimer=null;function requestOrientationPermission(){}
function saveGraphData(){localStorage.setItem("espuigraphs",JSON.stringify(graphData));}
function restoreGraphData(id){var savedData=localStorage.getItem("espuigraphs",graphData);if(savedData!=null){savedData=JSON.parse(savedData);let idData=savedData[id];return Array.isArray(idData)?idData:[];}
return[];}
function restart(){$(document).add("*").off();$("#row").html("");conStatusError();start();}
function conStatusError(){FragmentAssemblyTimer.forEach(element=>{clearInterval(element);});FragmentAssemblyTimer=new Array();controlAssemblyArray=new Array();if(true===websockConnected){websockConnected=false;websock.close();$("#conStatus").removeClass("color-green");$("#conStatus").addClass("color-red");$("#conStatus").html("Error / No Connection &#8635;");$("#conStatus").off();$("#conStatus").on({click:restart,});}}
function handleVisibilityChange(){if(!websockConnected&&!document.hidden){restart();}}
function start(){let location=window.location.hostname;let port=window.location.port;document.addEventListener("visibilitychange",handleVisibilityChange,false);if(port!=""||port!=80||port!=443){websock=new WebSocket("ws://"+location+":"+port+"/ws");}else{websock=new WebSocket("ws://"+location+"/ws");}
if(null===WebSocketTimer){WebSocketTimer=setInterval(function(){if(websock.readyState===3){restart();}},5000);}
websock.onopen=function(evt){console.log("websock open");$("#conStatus").addClass("color-green");$("#conStatus").text("Connected");websockConnected=true;FragmentAssemblyTimer.forEach(element=>{clearInterval(element);});FragmentAssemblyTimer=new Array();controlAssemblyArray=new Array();};websock.onclose=function(evt){console.log("websock close");conStatusError();FragmentAssemblyTimer.forEach(element=>{clearInterval(element);});FragmentAssemblyTimer=new Array();controlAssemblyArray=new Array();};websock.onerror=function(evt){console.log("websock Error");restart();FragmentAssemblyTimer.forEach(element=>{clearInterval(element);});FragmentAssemblyTimer=new Array();controlAssemblyArray=new Array();};var handleEvent=function(evt){try{var data=JSON.parse(evt.data);}
catch(Event){console.error(Event);websock.send("uiok:"+0);return;}
var e=document.body;var center="";switch(data.type){case UI_INITIAL_GUI:$("#row").html("");$("#tabsnav").html("");$("#tabscontent").html("");if(data.sliderContinuous){sliderContinuous=data.sliderContinuous;}
data.controls.forEach(element=>{var fauxEvent={data:JSON.stringify(element),};handleEvent(fauxEvent);});if(data.totalcontrols>(data.controls.length-1)){websock.send("uiok:"+(data.controls.length-1));}
break;case UI_EXTEND_GUI:data.controls.forEach(element=>{var fauxEvent={data:JSON.stringify(element),};handleEvent(fauxEvent);});if(data.totalcontrols>data.startindex+(data.controls.length-1)){websock.send("uiok:"+(data.startindex+(data.controls.length-1)));}
break;case UI_RELOAD:window.location.reload();break;case UI_TITEL:document.title=data.label;$("#mainHeader").html(data.label);break;case UI_LABEL:case UI_NUMBER:case UI_TEXT_INPUT:case UI_SELECT:case UI_GAUGE:case UI_SEPARATOR:if(data.visible)addToHTML(data);break;case UI_BUTTON:if(data.visible){addToHTML(data);$("#btn"+data.id).on({touchstart:function(e){e.preventDefault();buttonclick(data.id,true);},touchend:function(e){e.preventDefault();buttonclick(data.id,false);},});}
break;case UI_SWITCHER:if(data.visible){addToHTML(data);switcher(data.id,data.value);}
break;case UI_CPAD:case UI_PAD:if(data.visible){addToHTML(data);$("#pf"+data.id).on({touchstart:function(e){e.preventDefault();padclick(UP,data.id,true);},touchend:function(e){e.preventDefault();padclick(UP,data.id,false);},});$("#pl"+data.id).on({touchstart:function(e){e.preventDefault();padclick(LEFT,data.id,true);},touchend:function(e){e.preventDefault();padclick(LEFT,data.id,false);},});$("#pr"+data.id).on({touchstart:function(e){e.preventDefault();padclick(RIGHT,data.id,true);},touchend:function(e){e.preventDefault();padclick(RIGHT,data.id,false);},});$("#pb"+data.id).on({touchstart:function(e){e.preventDefault();padclick(DOWN,data.id,true);},touchend:function(e){e.preventDefault();padclick(DOWN,data.id,false);},});$("#pc"+data.id).on({touchstart:function(e){e.preventDefault();padclick(CENTER,data.id,true);},touchend:function(e){e.preventDefault();padclick(CENTER,data.id,false);},});}
break;case UI_SLIDER:if(data.visible){addToHTML(data);rangeSlider(!sliderContinuous);}
break;case UI_TAB:if(data.visible){$("#tabsnav").append("<li><a onmouseup='tabclick("+data.id+")' href='#tab"+data.id+"'>"+data.value+"</a></li>");$("#tabscontent").append("<div id='tab"+data.id+"'></div>");tabs=$(".tabscontent").tabbedContent({loop:true}).data("api");$("a").filter(function(){return $(this).attr("href")==="#click-to-switch";}).on("click",function(e){var tab=prompt("Tab to switch to (number or id)?");if(!tabs.switchTab(tab)){alert("That tab does not exist :\\");}
e.preventDefault();});}
break;case UI_OPTION:if(data.parentControl){var parent=$("#select"+data.parentControl);parent.append("<option id='option"+
data.id+
"' value='"+
data.value+
"' "+
data.selected+
">"+
data.label+
"</option>");}
break;case UI_MIN:if(data.parentControl){if($('#sl'+data.parentControl).length){$('#sl'+data.parentControl).attr("min",data.value);}else if($('#num'+data.parentControl).length){$('#num'+data.parentControl).attr("min",data.value);}}
break;case UI_MAX:if(data.parentControl){if($('#sl'+data.parentControl).length){$('#sl'+data.parentControl).attr("max",data.value);}else if($('#text'+data.parentControl).length){$('#text'+data.parentControl).attr("maxlength",data.value);}else if($('#num'+data.parentControl).length){$('#num'+data.parentControl).attr("max",data.value);}}
break;case UI_STEP:if(data.parentControl){if($('#sl'+data.parentControl).length){$('#sl'+data.parentControl).attr("step",data.value);}else if($('#num'+data.parentControl).length){$('#num'+data.parentControl).attr("step",data.value);}}
break;case UI_GRAPH:if(data.visible){addToHTML(data);graphData[data.id]=restoreGraphData(data.id);renderGraphSvg(graphData[data.id],"graph"+data.id);}
break;case ADD_GRAPH_POINT:var ts=new Date().getTime();graphData[data.id].push({x:ts,y:data.value});saveGraphData();renderGraphSvg(graphData[data.id],"graph"+data.id);break;case CLEAR_GRAPH:graphData[data.id]=[];saveGraphData();renderGraphSvg(graphData[data.id],"graph"+data.id);break;case UI_ACCEL:if(hasAccel)break;hasAccel=true;if(data.visible){addToHTML(data);requestOrientationPermission();}
break;case UI_FILEDISPLAY:if(data.visible)
{addToHTML(data);FileDisplayUploadFile(data);}
break;case UPDATE_LABEL:$("#l"+data.id).html(data.value);if(data.hasOwnProperty('elementStyle')){$("#l"+data.id).attr("style",data.elementStyle);}
break;case UPDATE_SWITCHER:switcher(data.id,data.value=="0"?0:1);if(data.hasOwnProperty('elementStyle')){$("#sl"+data.id).attr("style",data.elementStyle);}
break;case UPDATE_SLIDER:$("#sl"+data.id).attr("value",data.value)
slider_move($("#sl"+data.id).parent().parent(),data.value,"100",false);if(data.hasOwnProperty('elementStyle')){$("#sl"+data.id).attr("style",data.elementStyle);}
break;case UPDATE_NUMBER:$("#num"+data.id).val(data.value);if(data.hasOwnProperty('elementStyle')){$("#num"+data.id).attr("style",data.elementStyle);}
break;case UPDATE_TEXT_INPUT:$("#text"+data.id).val(data.value);if(data.hasOwnProperty('elementStyle')){$("#text"+data.id).attr("style",data.elementStyle);}
if(data.hasOwnProperty('inputType')){$("#text"+data.id).attr("type",data.inputType);}
break;case UPDATE_SELECT:$("#select"+data.id).val(data.value);if(data.hasOwnProperty('elementStyle')){$("#select"+data.id).attr("style",data.elementStyle);}
break;case UPDATE_BUTTON:$("#btn"+data.id).val(data.value);$("#btn"+data.id).text(data.value);if(data.hasOwnProperty('elementStyle')){$("#btn"+data.id).attr("style",data.elementStyle);}
break;case UPDATE_PAD:case UPDATE_CPAD:break;case UPDATE_GAUGE:$("#gauge"+data.id).val(data.value);if(data.hasOwnProperty('elementStyle')){$("#gauge"+data.id).attr("style",data.elementStyle);}
break;case UPDATE_ACCEL:break;case UPDATE_TIME:var rv=new Date().toISOString();websock.send("time:"+rv+":"+data.id);break;case UPDATE_FILEDISPLAY:FileDisplayUploadFile(data);break;case UI_FRAGMENT:let FragmentLen=data.length;let FragementOffset=data.offset;let NextFragmentOffset=FragementOffset+FragmentLen;let Total=data.total;let Arrived=(FragmentLen+FragementOffset);let FragmentFinal=Total===Arrived;if(!data.hasOwnProperty('control'))
{console.error("UI_FRAGMENT:Missing control record, skipping control");break;}
let control=data.control;StopFragmentAssemblyTimer(data.control.id);if(0===FragementOffset)
{controlAssemblyArray[control.id]=data;controlAssemblyArray[control.id].offset=NextFragmentOffset;StartFragmentAssemblyTimer(control.id);let TotalRequest=JSON.stringify({'id':control.id,'offset':NextFragmentOffset});websock.send("uifragmentok:"+0+": "+TotalRequest+":");break;}
if("undefined"===typeof controlAssemblyArray[control.id])
{console.error("Missing first fragment for control: "+control.id);StartFragmentAssemblyTimer(control.id);let TotalRequest=JSON.stringify({'id':control.id,'offset':0});websock.send("uifragmentok:"+0+": "+TotalRequest+":");break;}
if(FragementOffset!==controlAssemblyArray[control.id].offset)
{console.error("Wrong next fragment. Expected: "+controlAssemblyArray[control.id].offset+" Got: "+FragementOffset);StartFragmentAssemblyTimer(control.id);let TotalRequest=JSON.stringify({'id':control.id,'offset':controlAssemblyArray[control.id].length+controlAssemblyArray[control.id].offset});websock.send("uifragmentok:"+0+": "+TotalRequest+":");break;}
controlAssemblyArray[control.id].control.value+=control.value;controlAssemblyArray[control.id].offset=NextFragmentOffset;if(true===FragmentFinal)
{var fauxEvent={data:JSON.stringify(controlAssemblyArray[control.id].control),};handleEvent(fauxEvent);controlAssemblyArray[control.id]=null;}
else
{StartFragmentAssemblyTimer(control.id);let TotalRequest=JSON.stringify({'id':control.id,'offset':NextFragmentOffset});websock.send("uifragmentok:"+0+": "+TotalRequest+":");}
break;default:console.error("Unknown type or event");break;}
if(data.type>=UI_TITEL&&data.type<UPDATE_OFFSET){processEnabled(data);}
if(data.type>=UPDATE_OFFSET&&data.type<UI_INITIAL_GUI){var element=$("#id"+data.id);if(data.hasOwnProperty('panelStyle')){$("#id"+data.id).attr("style",data.panelStyle);}
if(data.hasOwnProperty('visible')){if(data['visible'])
$("#id"+data.id).show();else
$("#id"+data.id).hide();}
if(data.type==UPDATE_SLIDER){element.removeClass("slider-turquoise slider-emerald slider-peterriver slider-wetasphalt slider-sunflower slider-carrot slider-alizarin");element.addClass("slider-"+colorClass(data.color));}else{element.removeClass("turquoise emerald peterriver wetasphalt sunflower carrot alizarin");element.addClass(colorClass(data.color));}
processEnabled(data);}
$(".range-slider__range").each(function(){$(this)[0].value=$(this).attr("value");$(this).next().html($(this).attr("value"));});};websock.onmessage=handleEvent;}
async function FileDisplayUploadFile(data)
{let text=await downloadFile(data.value);let ItemToUpdateId="fd"+data.id;$("#"+ItemToUpdateId).val(text);$("#"+ItemToUpdateId).css("textAlign","left");$("#"+ItemToUpdateId).css("white-space","nowrap");$("#"+ItemToUpdateId).css("overflow","scroll");$("#"+ItemToUpdateId).css("overflow-y","scroll");$("#"+ItemToUpdateId).css("overflow-x","scroll");$("#"+ItemToUpdateId).scrollTop($("#"+ItemToUpdateId).val().length);}
async function downloadFile(filename)
{let response=await fetch(filename);if(response.status!=200){throw new Error("File Read Server Error: '"+response.status+"'");}
let text_data=await response.text();return text_data;}
function StartFragmentAssemblyTimer(Id)
{StopFragmentAssemblyTimer(Id);FragmentAssemblyTimer[Id]=setInterval(function(_Id)
{if("undefined"!==typeof controlAssemblyArray[_Id])
{if(null!==controlAssemblyArray[_Id])
{let TotalRequest=JSON.stringify({'id':controlAssemblyArray[_Id].control.id,'offset':controlAssemblyArray[_Id].offset});websock.send("uifragmentok:"+0+": "+TotalRequest+":");}}},1000,Id);}
function StopFragmentAssemblyTimer(Id)
{if("undefined"!==typeof FragmentAssemblyTimer[Id])
{if(FragmentAssemblyTimer[Id])
{clearInterval(FragmentAssemblyTimer[Id]);}}}
function sliderchange(number){var val=$("#sl"+number).val();websock.send("slvalue:"+val+":"+number);$(".range-slider__range").each(function(){$(this).attr("value",$(this)[0].value);});}
function numberchange(number){var val=$("#num"+number).val();websock.send("nvalue:"+val+":"+number);}
function textchange(number){var val=$("#text"+number).val();websock.send("tvalue:"+val+":"+number);}
function tabclick(number){var val=$("#tab"+number).val();websock.send("tabvalue:"+val+":"+number);}
function selectchange(number){var val=$("#select"+number).val();websock.send("svalue:"+val+":"+number);}
function buttonclick(number,isdown){if(isdown)websock.send("bdown:"+number);else websock.send("bup:"+number);}
function padclick(type,number,isdown){if($("#id"+number+" nav").hasClass("disabled")){return;}
switch(type){case CENTER:if(isdown)websock.send("pcdown:"+number);else websock.send("pcup:"+number);break;case UP:if(isdown)websock.send("pfdown:"+number);else websock.send("pfup:"+number);break;case DOWN:if(isdown)websock.send("pbdown:"+number);else websock.send("pbup:"+number);break;case LEFT:if(isdown)websock.send("pldown:"+number);else websock.send("plup:"+number);break;case RIGHT:if(isdown)websock.send("prdown:"+number);else websock.send("prup:"+number);break;}}
function switcher(number,state){if(state==null){if(!$("#sl"+number).hasClass("checked")){websock.send("sactive:"+number);$("#sl"+number).addClass("checked");}else{websock.send("sinactive:"+number);$("#sl"+number).removeClass("checked");}}else if(state==1){$("#sl"+number).addClass("checked");$("#sl"+number).prop("checked",true);}else if(state==0){$("#sl"+number).removeClass("checked");$("#sl"+number).prop("checked",false);}}
var rangeSlider=function(isDiscrete){var range=$(".range-slider__range");var slidercb=function(){sliderchange($(this).attr("id").replace(/^\D+/g,""));};range.on({input:function(){$(this).next().html(this.value)}});range.each(function(){$(this).next().html(this.value);if($(this).attr("callbackSet")!="true"){if(!isDiscrete){$(this).on({input:slidercb});}else{$(this).on({change:slidercb});}
$(this).attr("callbackSet","true");}});};var addToHTML=function(data){panelStyle=data.hasOwnProperty('panelStyle')?" style='"+data.panelStyle+"' ":"";panelwide=data.hasOwnProperty('wide')?"wide":"";if(!data.hasOwnProperty('parentControl')||$("#tab"+data.parentControl).length>0){var parent=data.hasOwnProperty('parentControl')?$("#tab"+data.parentControl):$("#row");var html="";switch(data.type){case UI_LABEL:case UI_BUTTON:case UI_SWITCHER:case UI_CPAD:case UI_PAD:case UI_SLIDER:case UI_NUMBER:case UI_TEXT_INPUT:case UI_SELECT:case UI_GRAPH:case UI_GAUGE:case UI_ACCEL:case UI_FILEDISPLAY:html="<div id='id"+data.id+"' "+panelStyle+" class='two columns "+panelwide+" card tcenter "+
colorClass(data.color)+"'><h5>"+data.label+"</h5><hr/>"+
elementHTML(data)+
"</div>";break;case UI_SEPARATOR:html="<div id='id"+data.id+"' "+panelStyle+" class='sectionbreak columns'>"+
"<h5>"+data.label+"</h5><hr/></div>";break;case UI_TIME:break;}
parent.append(html);}else{var parent=$("#id"+data.parentControl);parent.append(elementHTML(data));}}
var elementHTML=function(data){var id=data.id
var elementStyle=data.hasOwnProperty('elementStyle')?" style='"+data.elementStyle+"' ":"";var inputType=data.hasOwnProperty('inputType')?" type='"+data.inputType+"' ":"";switch(data.type){case UI_LABEL:return"<span id='l"+id+"' "+elementStyle+
" class='label label-wrap'>"+data.value+"</span>";case UI_FILEDISPLAY:return"<textarea id='fd"+id+"' rows='4' "+elementStyle+
" class='label label-wrap'>"+"</textarea>";case UI_BUTTON:return"<button id='btn"+id+"' "+elementStyle+
" onmousedown='buttonclick("+id+", true)'"+
" onmouseup='buttonclick("+id+", false)'>"+
data.value+"</button>";case UI_SWITCHER:return"<label id='sl"+id+"' "+elementStyle+
" class='switch "+(data.value=="1"?"checked":"")+
(data.hasOwnProperty('vertical')?" vert-switcher ":"")+
"'>"+
"<div class='in'>"+
"<input type='checkbox' id='s"+id+"' onClick='switcher("+id+",null)' "+
(data.value=="1"?"checked":"")+"/></div></label>";case UI_CPAD:case UI_PAD:return"<nav class='control'><ul>"+
"<li><a onmousedown='padclick(UP, "+id+", true)' "+
"onmouseup='padclick(UP, "+id+", false)' id='pf"+id+"'>&#9650;</a></li>"+
"<li><a onmousedown='padclick(RIGHT, "+id+", true)' "+
"onmouseup='padclick(RIGHT, "+id+", false)' id='pr"+id+"'>&#9650;</a></li>"+
"<li><a onmousedown='padclick(LEFT, "+id+", true)' "+
"onmouseup='padclick(LEFT, "+id+", false)' id='pl"+id+"'>&#9650;</a></li>"+
"<li><a onmousedown='padclick(DOWN, "+id+", true)' "+
"onmouseup='padclick(DOWN, "+id+", false)' id='pb"+id+"'>&#9650;</a></li>"+
"</ul>"+
(data.type==UI_CPAD?"<a class='confirm' onmousedown='padclick(CENTER,"+id+", true)' "+
"onmouseup='padclick(CENTER, "+id+", false)' id='pc"+id+"'>OK</a>":"")+
"</nav>";case UI_SLIDER:return"<div class='range-slider "+
(data.hasOwnProperty('vertical')?" vert-slider ":"")+
"'>"+
"<input id='sl"+id+"' type='range' min='0' max='100' value='"+data.value+"' "+
elementStyle+" class='range-slider__range'><span class='range-slider__value'>"+
data.value+"</span></div>";case UI_NUMBER:return"<input style='color:black; "+data.elementStyle+"' id='num"+id+
"' type='number' value='"+data.value+"' onchange='numberchange("+id+")' />";case UI_TEXT_INPUT:return"<input "+inputType+"style='color:black; "+data.elementStyle+"' id='text"+id+
"' value='"+data.value+"' onchange='textchange("+id+")' />";case UI_SELECT:return"<select style='color:black; "+data.elementStyle+"' id='select"+id+
"' onchange='selectchange("+id+")' />";case UI_GRAPH:return"<figure id='graph"+id+"'><figcaption>"+data.label+"</figcaption></figure>";case UI_GAUGE:return"WILL BE A GAUGE <input style='color:black;' id='gauge"+id+
"' type='number' value='"+data.value+"' onchange='numberchange("+id+")' />";case UI_ACCEL:return"ACCEL // Not implemented fully!<div class='accelerometer' id='accel"+id+
"' ><div class='ball"+id+"'></div><pre class='accelerometeroutput"+id+"'></pre>";default:return"";}}
var processEnabled=function(data){switch(data.type){case UI_SWITCHER:case UPDATE_SWITCHER:if(data.enabled){$("#sl"+data.id).removeClass('disabled');$("#s"+data.id).prop("disabled",false);}else{$("#sl"+data.id).addClass('disabled');$("#s"+data.id).prop("disabled",true);}
break;case UI_SLIDER:case UPDATE_SLIDER:$("#sl"+data.id).prop("disabled",!data.enabled);break;case UI_NUMBER:case UPDATE_NUMBER:$("#num"+data.id).prop("disabled",!data.enabled);break;case UI_TEXT_INPUT:case UPDATE_TEXT_INPUT:$("#text"+data.id).prop("disabled",!data.enabled);break;case UI_SELECT:case UPDATE_SELECT:$("#select"+data.id).prop("disabled",!data.enabled);break;case UI_BUTTON:case UPDATE_BUTTON:$("#btn"+data.id).prop("disabled",!data.enabled);break;case UI_PAD:case UI_CPAD:case UPDATE_PAD:case UPDATE_CPAD:case UI_FILEDISPLAY:case UPDATE_FILEDISPLAY:if(data.enabled){$("#id"+data.id+" nav").removeClass('disabled');}else{$("#id"+data.id+" nav").addClass('disabled');}
break;}}
)=====";

const uint8_t JS_CONTROLS_GZIP[5160] PROGMEM = {
	31, 139, 8, 0, 3, 199, 199, 101, 2, 255, 213, 60, 107, 119, 218, 74, 146, 223, 253, 43, 100, 229, 158, 0, 107, 108,
	67, 94, 147, 128, 229, 28, 130, 73, 194, 12, 126, 172, 141, 39, 119, 54, 55, 235, 35, 80, 99, 52, 22, 146, 70, 18,
	126, 44, 151, 255, 190, 85, 253, 82, 183, 30, 128, 237, 155, 217, 187, 95, 98, 84, 170, 174, 174, 174, 71, 119, 85,
	117, 41, 227, 192, 143, 19, 227, 178, 127, 213, 63, 233, 15, 251, 157, 193, 213, 151, 203, 190, 245, 170, 209, 104,
	143, 197, 139, 243, 222, 224, 180, 115, 4, 176, 166, 128, 157, 29, 117, 134, 189, 171, 211, 207, 159, 47, 122, 67,
	171, 169, 226, 246, 126, 29, 246, 78, 142, 24, 141, 166, 2, 31, 246, 135, 189, 129, 165, 0, 206, 128, 98, 134, 30,
	5, 165, 147, 244, 175, 186, 8, 121, 165, 35, 117, 25, 214, 171, 20, 235, 211, 229, 112, 120, 122, 98, 189, 214, 241,
	56, 180, 217, 120, 157, 98, 14, 58, 159, 128, 137, 55, 58, 34, 3, 54, 27, 111, 82, 188, 139, 111, 253, 97, 247, 107,
	239, 220, 122, 171, 163, 74, 120, 179, 241, 86, 193, 30, 244, 143, 0, 246, 46, 131, 203, 160, 205, 198, 187, 20,
	243, 228, 242, 248, 19, 192, 254, 162, 99, 114, 104, 179, 241, 23, 69, 92, 32, 71, 80, 200, 217, 229, 208, 122, 175,
	99, 43, 111, 154, 141, 247, 233, 136, 47, 231, 157, 179, 175, 214, 7, 14, 232, 28, 29, 49, 200, 213, 217, 105, 255,
	4, 81, 249, 139, 238, 160, 215, 57, 231, 200, 205, 198, 7, 101, 198, 206, 39, 171, 153, 81, 8, 131, 41, 10, 185,
	232, 13, 122, 93, 160, 150, 209, 137, 0, 55, 21, 173, 156, 158, 13, 251, 40, 255, 140, 90, 4, 184, 169, 168, 229,
	184, 15, 128, 140, 86, 40, 172, 169, 232, 228, 184, 243, 171, 213, 204, 232, 131, 194, 154, 170, 42, 134, 189, 51,
	171, 153, 213, 4, 5, 54, 21, 61, 124, 233, 92, 126, 233, 89, 205, 140, 30, 56, 180, 169, 168, 161, 211, 237, 162,
	109, 100, 84, 192, 161, 205, 247, 170, 96, 206, 58, 231, 157, 225, 41, 168, 241, 67, 86, 54, 242, 77, 83, 21, 119,
	255, 184, 7, 30, 149, 145, 55, 2, 155, 175, 20, 47, 249, 220, 31, 244, 142, 250, 23, 103, 131, 206, 63, 192, 159,
	116, 108, 245, 93, 243, 149, 162, 165, 207, 231, 157, 47, 199, 61, 208, 250, 135, 148, 113, 233, 122, 71, 167, 223,
	78, 164, 227, 13, 122, 159, 135, 210, 193, 206, 251, 95, 190, 14, 165, 27, 117, 129, 0, 88, 165, 208, 64, 247, 106,
	120, 121, 254, 159, 151, 167, 253, 139, 158, 36, 213, 189, 234, 29, 247, 206, 59, 131, 212, 145, 187, 87, 103, 61,
	24, 117, 222, 255, 59, 12, 125, 37, 129, 223, 122, 195, 206, 197, 217, 215, 206, 64, 33, 127, 117, 113, 121, 242,
	121, 112, 250, 77, 155, 163, 219, 57, 63, 63, 29, 74, 183, 235, 94, 117, 6, 253, 255, 234, 156, 131, 45, 188, 147,
	160, 163, 206, 249, 223, 164, 11, 117, 175, 78, 78, 79, 64, 144, 111, 223, 182, 111, 237, 200, 0, 96, 18, 5, 94, 39,
	142, 201, 108, 228, 61, 116, 162, 200, 126, 176, 124, 114, 103, 156, 142, 254, 73, 198, 73, 181, 70, 177, 62, 71,
	246, 245, 140, 248, 137, 64, 27, 186, 51, 18, 81, 52, 58, 128, 99, 93, 71, 118, 56, 61, 178, 19, 59, 247, 102, 106,
	199, 157, 241, 152, 120, 214, 196, 246, 98, 66, 65, 177, 231, 58, 36, 234, 194, 244, 174, 63, 15, 230, 49, 127, 53,
	153, 251, 227, 196, 13, 124, 96, 204, 11, 162, 174, 103, 199, 113, 149, 254, 236, 59, 181, 5, 255, 97, 157, 204,
	103, 35, 18, 73, 120, 59, 190, 115, 147, 241, 84, 193, 179, 99, 162, 202, 191, 21, 145, 100, 30, 249, 38, 252, 243,
	175, 121, 224, 198, 196, 108, 115, 20, 174, 14, 129, 64, 96, 89, 182, 231, 200, 215, 169, 110, 4, 70, 72, 18, 18,
	69, 238, 45, 137, 36, 82, 170, 43, 129, 116, 71, 18, 59, 14, 167, 182, 151, 72, 36, 169, 59, 129, 19, 207, 253, 137,
	23, 220, 41, 116, 152, 42, 197, 251, 177, 29, 69, 65, 58, 94, 168, 85, 188, 182, 61, 247, 127, 236, 200, 245, 37, 2,
	42, 185, 197, 127, 163, 134, 5, 162, 99, 71, 55, 102, 219, 33, 19, 123, 238, 37, 2, 104, 182, 151, 203, 45, 212,
	194, 29, 25, 197, 193, 248, 166, 173, 252, 6, 149, 248, 160, 121, 226, 40, 202, 250, 70, 70, 23, 240, 138, 36, 92,
	241, 115, 207, 75, 53, 21, 145, 127, 205, 73, 156, 156, 70, 46, 88, 136, 141, 160, 51, 18, 205, 220, 56, 134, 95,
	213, 218, 98, 185, 37, 49, 99, 251, 150, 124, 17, 70, 2, 175, 188, 96, 108, 123, 23, 73, 0, 214, 69, 246, 98, 146,
	244, 19, 50, 171, 154, 36, 14, 231, 46, 181, 165, 216, 172, 255, 245, 226, 244, 100, 47, 78, 96, 165, 215, 238, 228,
	161, 42, 77, 172, 86, 107, 43, 132, 35, 152, 63, 136, 20, 218, 46, 88, 1, 181, 50, 152, 209, 161, 38, 169, 205, 117,
	93, 56, 87, 74, 188, 237, 78, 170, 114, 232, 54, 93, 111, 109, 145, 210, 162, 76, 133, 118, 20, 147, 20, 171, 214,
	246, 72, 98, 184, 12, 65, 66, 191, 187, 206, 143, 54, 19, 58, 115, 136, 61, 55, 102, 142, 193, 48, 107, 31, 217,
	223, 214, 247, 31, 176, 30, 134, 72, 127, 106, 75, 179, 35, 112, 196, 197, 47, 85, 39, 24, 207, 209, 13, 107, 123,
	182, 227, 84, 205, 255, 48, 107, 123, 193, 100, 2, 62, 246, 75, 213, 124, 17, 5, 119, 240, 60, 77, 102, 94, 213, 52,
	107, 232, 237, 23, 160, 141, 121, 220, 3, 59, 138, 0, 135, 147, 81, 105, 103, 113, 22, 133, 126, 190, 55, 9, 162,
	158, 13, 254, 69, 60, 130, 47, 173, 195, 197, 216, 35, 118, 212, 247, 193, 21, 110, 109, 79, 192, 129, 116, 173,
	189, 126, 167, 40, 221, 113, 4, 2, 200, 62, 137, 230, 196, 178, 172, 172, 69, 214, 22, 37, 54, 202, 193, 123, 99,
	47, 0, 157, 48, 121, 200, 197, 129, 84, 34, 50, 11, 110, 9, 219, 77, 76, 186, 77, 236, 94, 71, 132, 248, 102, 30,
	21, 36, 171, 225, 69, 196, 41, 192, 98, 98, 166, 98, 51, 246, 141, 147, 192, 224, 28, 161, 84, 95, 190, 120, 255,
	238, 245, 219, 118, 193, 168, 84, 89, 26, 212, 175, 130, 64, 221, 241, 77, 139, 235, 186, 14, 114, 92, 42, 106, 154,
	218, 190, 227, 145, 191, 187, 177, 59, 114, 61, 55, 121, 232, 2, 224, 26, 150, 185, 0, 73, 109, 103, 37, 242, 242,
	229, 182, 48, 147, 189, 169, 235, 56, 196, 175, 45, 164, 13, 105, 100, 133, 93, 161, 221, 162, 119, 32, 208, 186,
	115, 125, 39, 184, 219, 19, 207, 123, 211, 32, 78, 124, 123, 70, 168, 117, 135, 65, 148, 228, 48, 16, 216, 150, 83,
	130, 248, 122, 183, 240, 99, 224, 198, 9, 241, 97, 147, 54, 111, 37, 223, 99, 202, 183, 89, 47, 94, 79, 157, 170,
	146, 170, 31, 73, 110, 91, 166, 249, 251, 239, 236, 215, 251, 134, 248, 245, 230, 205, 107, 105, 4, 212, 104, 228,
	206, 84, 53, 239, 226, 214, 254, 190, 185, 35, 24, 219, 49, 91, 230, 14, 142, 218, 49, 247, 239, 64, 206, 237, 37,
	1, 250, 27, 15, 230, 99, 182, 128, 29, 116, 127, 176, 70, 125, 19, 172, 45, 50, 155, 34, 238, 95, 194, 35, 132, 140,
	153, 142, 132, 117, 70, 196, 118, 30, 80, 237, 104, 219, 175, 117, 181, 212, 223, 54, 26, 13, 156, 79, 32, 7, 126,
	16, 18, 223, 146, 148, 200, 109, 130, 7, 160, 31, 7, 30, 1, 225, 95, 3, 203, 12, 211, 64, 188, 13, 12, 185, 204,
	224, 19, 114, 15, 235, 151, 246, 3, 24, 57, 39, 67, 119, 108, 255, 41, 246, 134, 101, 59, 149, 15, 117, 246, 77, 4,
	68, 17, 139, 182, 196, 63, 221, 146, 8, 50, 182, 201, 146, 232, 10, 96, 73, 169, 9, 253, 73, 214, 194, 162, 61, 244,
	111, 186, 13, 100, 214, 146, 68, 15, 244, 88, 118, 50, 167, 40, 188, 220, 115, 232, 33, 186, 220, 2, 7, 4, 134, 233,
	232, 116, 241, 84, 48, 28, 40, 229, 21, 19, 31, 78, 193, 185, 27, 220, 128, 167, 55, 106, 252, 156, 109, 179, 200,
	134, 88, 114, 79, 26, 5, 206, 3, 11, 118, 9, 174, 27, 54, 22, 17, 49, 226, 156, 123, 201, 67, 72, 120, 204, 168,
	231, 244, 173, 130, 99, 21, 65, 137, 61, 138, 125, 251, 182, 8, 140, 18, 130, 73, 212, 87, 176, 1, 208, 105, 178, 1,
	47, 4, 20, 217, 16, 184, 16, 15, 214, 67, 225, 92, 248, 113, 129, 78, 113, 109, 16, 223, 221, 51, 145, 47, 16, 189,
	149, 9, 156, 132, 162, 235, 203, 182, 162, 158, 170, 28, 69, 45, 64, 176, 154, 4, 137, 237, 137, 249, 14, 171, 250,
	244, 30, 241, 175, 147, 233, 110, 179, 38, 55, 98, 93, 17, 165, 232, 176, 144, 17, 108, 129, 55, 109, 33, 234, 180,
	242, 209, 250, 191, 93, 33, 147, 59, 58, 18, 156, 109, 228, 126, 231, 73, 43, 222, 100, 124, 94, 6, 172, 82, 212,
	202, 30, 170, 17, 241, 2, 219, 1, 143, 210, 177, 105, 77, 168, 37, 13, 59, 113, 19, 143, 48, 171, 241, 236, 17, 241,
	168, 25, 206, 108, 215, 255, 10, 39, 13, 100, 22, 220, 10, 83, 132, 44, 61, 90, 201, 105, 137, 39, 86, 86, 145, 143,
	105, 221, 68, 130, 88, 217, 66, 62, 210, 252, 95, 121, 201, 243, 246, 150, 144, 50, 61, 250, 61, 82, 131, 163, 104,
	24, 124, 29, 30, 15, 170, 204, 201, 117, 38, 88, 221, 41, 55, 104, 145, 29, 133, 139, 27, 37, 190, 185, 67, 209, 32,
	194, 167, 145, 83, 18, 204, 199, 83, 42, 251, 86, 186, 215, 212, 22, 100, 47, 140, 8, 170, 253, 136, 101, 62, 40,
	202, 121, 146, 224, 169, 1, 145, 86, 149, 147, 168, 227, 201, 6, 74, 169, 83, 42, 160, 211, 167, 208, 224, 113, 203,
	146, 134, 109, 25, 245, 138, 250, 215, 250, 213, 177, 29, 9, 2, 38, 65, 150, 161, 219, 30, 101, 48, 67, 22, 75, 122,
	82, 238, 248, 123, 35, 225, 133, 147, 39, 203, 46, 180, 29, 182, 232, 203, 179, 250, 83, 101, 87, 68, 67, 149, 29,
	101, 209, 123, 62, 139, 88, 162, 121, 62, 147, 26, 149, 28, 155, 209, 243, 217, 164, 197, 163, 231, 243, 169, 147,
	201, 49, 58, 122, 62, 163, 88, 255, 122, 62, 159, 26, 149, 28, 155, 227, 231, 179, 201, 202, 111, 207, 103, 52, 67,
	103, 165, 119, 211, 138, 245, 122, 231, 139, 48, 191, 185, 160, 231, 122, 117, 59, 23, 7, 228, 200, 14, 59, 159,
	242, 52, 245, 192, 195, 14, 67, 122, 2, 29, 120, 238, 225, 129, 109, 4, 254, 12, 40, 145, 121, 104, 85, 0, 135, 173,
	67, 74, 116, 199, 172, 85, 140, 105, 68, 38, 86, 5, 41, 40, 240, 202, 33, 127, 160, 219, 204, 142, 121, 176, 111,
	31, 30, 236, 3, 201, 194, 136, 70, 206, 233, 184, 183, 134, 235, 208, 169, 52, 98, 7, 251, 240, 6, 199, 226, 56, 11,
	8, 236, 233, 4, 224, 105, 68, 156, 46, 123, 174, 46, 188, 32, 8, 91, 168, 166, 101, 141, 6, 127, 85, 211, 14, 93,
	54, 179, 13, 216, 19, 215, 131, 96, 77, 77, 165, 120, 29, 229, 151, 106, 50, 117, 99, 224, 39, 73, 32, 189, 196,
	133, 153, 53, 72, 168, 32, 171, 193, 117, 239, 38, 193, 46, 219, 75, 77, 56, 240, 209, 152, 76, 10, 55, 235, 170, 1,
	96, 60, 1, 236, 88, 97, 20, 204, 66, 72, 127, 134, 246, 200, 72, 2, 131, 13, 196, 95, 144, 244, 97, 141, 209, 128,
	252, 30, 108, 242, 35, 139, 227, 182, 113, 61, 123, 12, 9, 70, 84, 225, 17, 98, 2, 219, 35, 17, 146, 152, 218, 9,
	210, 52, 156, 128, 196, 134, 31, 36, 6, 185, 135, 36, 216, 104, 253, 246, 27, 77, 36, 11, 108, 174, 192, 164, 88,
	181, 95, 170, 31, 194, 99, 24, 209, 101, 145, 4, 99, 155, 129, 80, 188, 47, 98, 136, 121, 198, 9, 215, 129, 142,
	218, 102, 143, 169, 210, 130, 144, 38, 253, 168, 55, 246, 211, 220, 217, 18, 186, 219, 50, 43, 6, 53, 1, 171, 34,
	160, 204, 34, 240, 133, 128, 176, 201, 8, 98, 31, 10, 24, 13, 42, 0, 112, 176, 207, 104, 30, 154, 249, 21, 29, 247,
	75, 151, 3, 224, 95, 170, 149, 23, 177, 87, 41, 90, 2, 143, 155, 208, 244, 203, 113, 152, 17, 204, 92, 223, 212, 15,
	76, 204, 245, 13, 78, 31, 84, 185, 126, 130, 82, 164, 178, 25, 114, 235, 236, 252, 250, 243, 215, 105, 223, 175, 88,
	39, 102, 242, 235, 103, 40, 199, 146, 115, 48, 244, 159, 45, 209, 220, 90, 114, 219, 235, 176, 119, 246, 211, 69,
	26, 39, 36, 252, 201, 43, 45, 152, 34, 187, 84, 122, 189, 184, 254, 32, 145, 21, 234, 239, 220, 117, 127, 88, 185,
	210, 183, 56, 71, 33, 29, 134, 100, 36, 162, 47, 46, 110, 175, 171, 249, 177, 117, 147, 194, 210, 163, 87, 247, 221,
	204, 141, 104, 139, 238, 153, 49, 205, 247, 129, 12, 169, 214, 176, 140, 142, 69, 130, 106, 17, 99, 123, 225, 60,
	158, 86, 23, 247, 173, 36, 174, 63, 180, 210, 197, 195, 158, 151, 185, 6, 120, 10, 163, 10, 155, 202, 253, 108, 171,
	64, 62, 223, 127, 252, 193, 243, 137, 75, 78, 212, 150, 184, 215, 170, 177, 247, 242, 154, 139, 22, 204, 214, 135,
	5, 43, 47, 78, 114, 27, 169, 114, 123, 153, 179, 148, 173, 28, 241, 207, 174, 71, 142, 220, 56, 244, 236, 135, 203,
	16, 147, 73, 4, 84, 69, 145, 69, 165, 172, 92, 234, 211, 138, 135, 26, 130, 167, 249, 35, 183, 92, 49, 49, 44, 245,
	244, 206, 63, 139, 130, 16, 14, 191, 135, 106, 133, 103, 223, 23, 201, 131, 71, 42, 53, 22, 174, 168, 132, 132, 39,
	192, 107, 238, 10, 234, 136, 98, 150, 100, 242, 180, 34, 55, 130, 115, 191, 97, 126, 108, 180, 154, 143, 99, 45,
	126, 54, 111, 44, 244, 43, 161, 69, 89, 211, 92, 126, 139, 133, 125, 87, 120, 15, 81, 205, 141, 98, 219, 70, 53,
	253, 161, 12, 173, 155, 205, 70, 195, 84, 170, 227, 255, 190, 69, 242, 130, 0, 210, 130, 45, 78, 33, 134, 165, 195,
	167, 90, 133, 78, 233, 41, 108, 41, 133, 9, 26, 165, 194, 129, 246, 7, 241, 150, 33, 181, 158, 185, 50, 226, 174,
	31, 206, 147, 225, 67, 184, 154, 50, 86, 30, 57, 97, 57, 160, 196, 220, 88, 225, 37, 23, 244, 61, 119, 193, 57, 98,
	79, 209, 7, 47, 224, 228, 235, 51, 89, 206, 242, 24, 244, 242, 225, 169, 204, 235, 164, 158, 194, 121, 90, 70, 73,
	91, 165, 90, 121, 52, 86, 231, 194, 41, 175, 237, 249, 53, 249, 131, 100, 159, 165, 245, 148, 5, 176, 147, 168, 192,
	69, 250, 199, 61, 122, 96, 71, 183, 234, 129, 157, 4, 253, 139, 211, 11, 90, 50, 173, 102, 235, 232, 9, 156, 228,
	45, 115, 39, 186, 165, 23, 103, 133, 39, 95, 174, 139, 166, 181, 234, 148, 201, 156, 94, 188, 197, 166, 133, 23,
	137, 226, 170, 97, 64, 124, 94, 190, 164, 209, 84, 91, 188, 163, 203, 62, 157, 76, 98, 146, 176, 247, 1, 253, 77,
	223, 159, 128, 201, 136, 241, 28, 37, 51, 100, 71, 33, 79, 135, 12, 177, 218, 107, 165, 133, 95, 10, 236, 208, 54,
	14, 199, 170, 42, 216, 59, 25, 74, 181, 182, 202, 237, 103, 215, 7, 50, 140, 152, 101, 241, 241, 52, 47, 44, 212,
	57, 175, 255, 130, 186, 183, 50, 247, 24, 166, 42, 143, 99, 60, 240, 253, 107, 209, 137, 99, 68, 100, 28, 68, 78,
	221, 136, 111, 220, 48, 84, 94, 152, 66, 162, 203, 45, 100, 138, 67, 45, 181, 212, 220, 190, 72, 130, 176, 240, 26,
	71, 171, 72, 83, 189, 2, 219, 13, 88, 69, 118, 193, 148, 213, 220, 37, 207, 247, 116, 228, 15, 58, 99, 123, 29, 22,
	215, 152, 149, 215, 22, 48, 105, 71, 73, 49, 151, 42, 131, 82, 113, 231, 44, 76, 178, 50, 245, 254, 69, 197, 117,
	42, 173, 116, 68, 189, 194, 166, 172, 180, 242, 115, 46, 243, 151, 70, 19, 142, 192, 46, 143, 192, 230, 33, 209, 85,
	167, 67, 47, 72, 37, 14, 210, 50, 231, 16, 53, 78, 92, 159, 56, 38, 200, 13, 183, 238, 96, 98, 172, 19, 67, 94, 245,
	66, 221, 19, 55, 138, 19, 67, 112, 97, 76, 2, 217, 139, 133, 156, 168, 130, 248, 233, 242, 106, 252, 17, 226, 201,
	24, 210, 182, 101, 109, 104, 34, 121, 17, 125, 139, 2, 16, 144, 15, 90, 148, 242, 217, 51, 122, 247, 33, 45, 63, 40,
	210, 89, 71, 121, 199, 52, 190, 4, 9, 14, 200, 185, 245, 79, 151, 233, 90, 22, 217, 142, 183, 233, 82, 158, 173,
	161, 181, 243, 136, 223, 172, 240, 99, 105, 143, 207, 241, 246, 180, 155, 71, 219, 70, 65, 235, 27, 220, 233, 109,
	202, 245, 138, 75, 191, 181, 219, 25, 109, 102, 91, 110, 97, 210, 191, 181, 248, 255, 180, 55, 137, 120, 64, 244,
	246, 101, 207, 24, 255, 198, 15, 238, 124, 3, 119, 42, 44, 99, 210, 218, 163, 238, 179, 242, 250, 251, 208, 18, 151,
	139, 47, 95, 74, 224, 129, 214, 180, 94, 91, 132, 81, 48, 38, 113, 220, 243, 109, 200, 56, 29, 153, 74, 102, 232,
	168, 99, 52, 98, 218, 213, 58, 43, 101, 138, 251, 93, 140, 134, 92, 71, 9, 58, 202, 194, 168, 208, 246, 137, 167, 5,
	81, 234, 176, 130, 8, 42, 29, 176, 42, 90, 231, 89, 52, 210, 228, 40, 223, 37, 12, 182, 240, 220, 60, 241, 52, 184,
	131, 248, 137, 154, 76, 238, 229, 20, 242, 188, 106, 86, 48, 150, 165, 229, 141, 181, 5, 95, 186, 222, 153, 198,
	114, 196, 93, 217, 166, 202, 155, 100, 119, 121, 91, 170, 120, 76, 123, 80, 5, 36, 109, 56, 21, 16, 217, 94, 42, 0,
	172, 159, 84, 60, 201, 246, 81, 92, 5, 17, 157, 91, 58, 27, 184, 207, 202, 30, 92, 30, 63, 192, 115, 77, 116, 82,
	21, 46, 33, 229, 93, 48, 173, 112, 171, 178, 41, 249, 227, 140, 173, 226, 168, 148, 143, 173, 18, 155, 196, 187, 7,
	122, 233, 178, 203, 211, 238, 43, 250, 100, 214, 246, 8, 246, 21, 40, 119, 10, 252, 50, 225, 123, 227, 7, 175, 39,
	232, 183, 11, 44, 145, 199, 204, 133, 65, 241, 80, 170, 242, 178, 72, 33, 38, 43, 234, 43, 45, 61, 51, 96, 15, 14,
	31, 75, 217, 158, 128, 65, 59, 126, 240, 199, 134, 108, 197, 91, 17, 70, 111, 209, 254, 60, 204, 147, 44, 251, 206,
	118, 19, 195, 1, 167, 214, 80, 68, 238, 129, 120, 216, 213, 58, 12, 46, 67, 128, 147, 190, 99, 153, 147, 212, 50,
	105, 246, 101, 238, 232, 24, 44, 131, 65, 234, 181, 146, 247, 99, 170, 85, 64, 232, 120, 238, 181, 111, 214, 77,
	143, 76, 18, 115, 37, 246, 221, 212, 77, 64, 242, 161, 61, 6, 55, 52, 97, 19, 138, 236, 112, 245, 8, 176, 159, 8,
	205, 1, 208, 227, 49, 108, 153, 222, 102, 232, 187, 15, 143, 29, 112, 191, 126, 0, 123, 61, 12, 194, 106, 185, 192,
	100, 249, 55, 175, 74, 77, 61, 19, 248, 7, 187, 39, 185, 22, 35, 18, 135, 176, 73, 19, 174, 201, 9, 193, 30, 36,
	137, 131, 187, 158, 192, 192, 110, 146, 100, 30, 111, 227, 103, 69, 181, 69, 50, 141, 130, 59, 3, 243, 56, 214, 185,
	102, 34, 113, 227, 156, 216, 142, 113, 65, 34, 244, 44, 10, 111, 25, 21, 200, 223, 116, 10, 59, 102, 133, 30, 21,
	194, 136, 174, 104, 235, 21, 155, 95, 162, 210, 44, 92, 244, 79, 165, 104, 106, 171, 240, 138, 131, 17, 132, 130, 7,
	103, 89, 230, 129, 13, 250, 133, 111, 190, 247, 225, 244, 45, 236, 154, 188, 162, 36, 245, 152, 123, 123, 117, 204,
	13, 67, 126, 176, 49, 120, 158, 151, 69, 159, 28, 235, 81, 199, 119, 158, 194, 222, 198, 1, 31, 197, 126, 102, 4,
	183, 92, 46, 235, 205, 70, 163, 81, 239, 59, 53, 93, 35, 43, 36, 94, 46, 189, 82, 85, 176, 49, 171, 94, 235, 173,
	131, 229, 152, 200, 178, 210, 102, 76, 119, 96, 214, 251, 203, 111, 85, 217, 233, 15, 68, 44, 81, 166, 228, 112,
	230, 92, 25, 65, 197, 30, 221, 224, 64, 70, 240, 151, 22, 39, 56, 118, 251, 209, 219, 188, 94, 158, 205, 238, 253,
	252, 66, 86, 114, 206, 166, 89, 193, 57, 45, 101, 174, 98, 221, 47, 227, 92, 153, 5, 221, 109, 197, 28, 172, 112,
	184, 106, 146, 100, 147, 73, 68, 87, 64, 225, 20, 120, 143, 191, 114, 6, 123, 180, 193, 28, 172, 150, 184, 74, 209,
	188, 216, 184, 82, 217, 27, 76, 164, 54, 97, 177, 215, 117, 55, 198, 125, 151, 134, 111, 252, 167, 78, 118, 132, 48,
	133, 26, 189, 226, 203, 160, 204, 195, 226, 233, 100, 103, 8, 122, 80, 61, 63, 161, 136, 0, 217, 27, 72, 58, 121,
	111, 168, 29, 243, 152, 200, 113, 99, 26, 158, 64, 116, 176, 144, 61, 170, 188, 3, 85, 105, 62, 101, 141, 39, 173,
	178, 37, 132, 227, 245, 107, 8, 199, 218, 34, 180, 218, 93, 57, 225, 201, 6, 132, 39, 101, 132, 177, 175, 167, 156,
	244, 6, 114, 15, 71, 101, 164, 177, 5, 171, 156, 180, 183, 1, 105, 175, 140, 52, 237, 154, 42, 167, 29, 109, 64, 59,
	42, 160, 173, 237, 123, 226, 210, 138, 91, 12, 158, 199, 132, 26, 76, 204, 62, 1, 96, 95, 21, 97, 249, 48, 187, 11,
	166, 166, 3, 4, 198, 55, 204, 114, 50, 126, 98, 195, 44, 183, 68, 223, 12, 53, 34, 74, 255, 191, 32, 162, 127, 2,
	33, 40, 65, 58, 190, 142, 150, 254, 253, 76, 74, 78, 94, 149, 243, 37, 53, 211, 139, 167, 85, 108, 100, 113, 32,
	138, 15, 211, 247, 162, 117, 43, 67, 187, 145, 167, 93, 194, 214, 26, 242, 162, 163, 139, 245, 137, 43, 189, 89,
	105, 179, 186, 27, 67, 52, 62, 6, 95, 229, 173, 66, 20, 201, 42, 61, 110, 148, 15, 26, 199, 35, 75, 57, 118, 180,
	179, 79, 63, 131, 96, 199, 192, 5, 64, 200, 63, 38, 213, 253, 255, 254, 237, 104, 103, 255, 186, 110, 210, 4, 130,
	245, 139, 209, 62, 56, 122, 45, 212, 42, 56, 200, 212, 68, 4, 33, 252, 4, 91, 46, 121, 183, 89, 233, 9, 88, 50, 176,
	77, 55, 50, 149, 193, 177, 237, 121, 35, 123, 124, 115, 65, 32, 220, 223, 182, 76, 212, 138, 201, 236, 85, 21, 143,
	24, 148, 114, 43, 4, 177, 20, 246, 166, 162, 48, 89, 104, 56, 91, 229, 243, 214, 249, 172, 237, 229, 82, 124, 92,
	32, 239, 185, 83, 57, 211, 60, 105, 145, 166, 250, 214, 218, 218, 193, 71, 211, 160, 85, 2, 236, 119, 202, 212, 9,
	118, 176, 229, 169, 101, 154, 109, 10, 186, 3, 54, 139, 201, 225, 27, 36, 132, 127, 41, 126, 233, 61, 128, 214, 19,
	82, 169, 253, 254, 187, 60, 115, 203, 251, 74, 14, 27, 90, 179, 215, 38, 116, 63, 174, 34, 155, 126, 202, 192, 190,
	208, 0, 237, 175, 254, 12, 66, 239, 12, 231, 55, 124, 185, 174, 230, 210, 126, 228, 76, 135, 228, 211, 59, 204, 105,
	91, 71, 113, 191, 57, 187, 251, 42, 234, 142, 96, 203, 147, 125, 138, 74, 121, 134, 106, 119, 71, 213, 182, 49, 198,
	237, 195, 170, 36, 119, 1, 126, 115, 60, 159, 249, 177, 192, 64, 213, 34, 130, 29, 57, 70, 194, 190, 29, 193, 110,
	184, 226, 106, 4, 109, 128, 156, 190, 21, 29, 149, 172, 51, 206, 60, 216, 7, 208, 193, 52, 218, 199, 150, 57, 94,
	215, 72, 155, 52, 104, 231, 28, 237, 153, 204, 92, 150, 165, 173, 244, 79, 89, 74, 204, 62, 68, 164, 36, 197, 154,
	176, 213, 19, 102, 91, 193, 96, 49, 39, 244, 34, 81, 148, 12, 245, 158, 66, 100, 77, 184, 120, 166, 49, 81, 178,
	185, 178, 41, 49, 39, 15, 185, 41, 43, 111, 178, 110, 142, 175, 65, 18, 92, 8, 42, 246, 10, 223, 215, 175, 95, 115,
	222, 175, 190, 150, 254, 79, 39, 18, 183, 242, 214, 186, 91, 126, 160, 73, 235, 124, 130, 164, 124, 37, 233, 173,
	243, 53, 254, 105, 246, 65, 12, 42, 165, 186, 134, 19, 76, 104, 89, 99, 112, 75, 42, 154, 234, 208, 160, 255, 238,
	98, 145, 37, 223, 207, 139, 196, 14, 205, 194, 22, 34, 49, 31, 38, 24, 160, 22, 155, 206, 137, 245, 34, 54, 41, 108,
	22, 48, 195, 155, 71, 206, 14, 83, 10, 122, 202, 180, 124, 251, 16, 51, 178, 248, 157, 206, 71, 175, 240, 203, 86,
	201, 187, 154, 49, 22, 3, 76, 37, 232, 103, 67, 234, 6, 13, 20, 176, 101, 213, 84, 59, 160, 139, 48, 217, 153, 95,
	57, 212, 219, 91, 129, 91, 134, 172, 240, 42, 247, 54, 193, 45, 91, 38, 50, 27, 175, 215, 8, 111, 32, 22, 223, 4,
	137, 22, 165, 166, 249, 81, 198, 31, 96, 11, 224, 248, 37, 133, 104, 248, 227, 194, 1, 72, 237, 9, 31, 118, 69, 20,
	105, 240, 113, 38, 119, 100, 220, 16, 248, 156, 174, 207, 97, 212, 230, 184, 25, 210, 217, 70, 193, 125, 133, 113,
	46, 24, 15, 252, 46, 202, 69, 48, 138, 223, 231, 50, 9, 209, 96, 148, 54, 251, 174, 225, 220, 20, 59, 197, 193, 62,
	149, 140, 34, 186, 220, 41, 32, 68, 8, 9, 145, 96, 86, 92, 138, 31, 30, 204, 61, 198, 181, 214, 193, 206, 116, 173,
	126, 41, 98, 232, 202, 70, 14, 77, 69, 217, 133, 168, 92, 219, 116, 233, 248, 209, 11, 107, 79, 127, 249, 226, 195,
	187, 183, 141, 118, 218, 228, 190, 110, 118, 246, 105, 197, 166, 12, 100, 176, 53, 30, 162, 39, 243, 64, 63, 67,
	217, 148, 5, 29, 89, 227, 192, 123, 50, 7, 244, 195, 141, 77, 57, 208, 145, 53, 14, 70, 43, 57, 216, 103, 230, 160,
	221, 152, 48, 139, 250, 104, 2, 107, 169, 245, 76, 220, 104, 86, 41, 97, 149, 127, 186, 177, 33, 175, 28, 187, 152,
	219, 177, 224, 246, 244, 111, 200, 168, 240, 190, 131, 125, 176, 101, 117, 183, 96, 241, 141, 48, 116, 197, 43, 213,
	68, 33, 117, 171, 13, 28, 158, 143, 208, 221, 157, 185, 182, 190, 9, 49, 71, 167, 243, 84, 140, 153, 11, 130, 104,
	192, 95, 251, 222, 170, 52, 27, 13, 165, 161, 95, 221, 240, 168, 52, 244, 227, 174, 136, 97, 158, 217, 128, 151,
	210, 211, 168, 16, 131, 82, 44, 216, 81, 233, 145, 35, 130, 137, 76, 224, 39, 228, 196, 150, 195, 15, 97, 26, 63,
	181, 70, 144, 8, 221, 180, 141, 146, 19, 25, 87, 78, 171, 109, 252, 107, 5, 182, 118, 150, 224, 149, 46, 21, 14, 1,
	154, 109, 8, 68, 158, 135, 49, 249, 129, 162, 247, 21, 6, 149, 80, 84, 103, 18, 176, 211, 163, 252, 145, 28, 179,
	218, 93, 246, 3, 139, 50, 38, 149, 66, 96, 33, 139, 60, 52, 150, 113, 2, 173, 166, 61, 86, 136, 162, 6, 199, 153,
	74, 103, 215, 170, 119, 133, 243, 179, 88, 92, 76, 63, 113, 175, 231, 17, 161, 52, 121, 179, 52, 255, 8, 8, 94, 140,
	109, 254, 53, 72, 38, 212, 84, 94, 209, 7, 32, 160, 210, 167, 209, 61, 167, 255, 173, 63, 24, 24, 159, 122, 70, 199,
	160, 96, 163, 220, 100, 216, 186, 120, 51, 221, 207, 50, 15, 150, 107, 112, 222, 232, 131, 177, 143, 255, 239, 7,
	248, 228, 44, 100, 98, 38, 142, 49, 129, 67, 244, 97, 91, 221, 5, 108, 108, 7, 39, 81, 48, 195, 75, 80, 198, 41, 5,
	73, 78, 15, 85, 236, 17, 164, 190, 82, 142, 236, 144, 13, 65, 198, 69, 180, 130, 121, 2, 242, 72, 145, 67, 42, 202,
	178, 255, 230, 71, 191, 38, 205, 134, 212, 229, 145, 105, 38, 209, 203, 180, 101, 139, 11, 110, 194, 200, 22, 52,
	29, 171, 5, 154, 138, 40, 131, 86, 120, 133, 70, 237, 127, 166, 21, 26, 89, 39, 149, 37, 26, 94, 62, 200, 182, 50,
	139, 146, 210, 99, 72, 242, 162, 82, 241, 23, 124, 155, 116, 118, 103, 9, 110, 107, 107, 207, 164, 77, 90, 162, 187,
	166, 153, 250, 81, 132, 115, 41, 243, 38, 45, 209, 143, 154, 65, 203, 192, 215, 119, 31, 63, 138, 182, 86, 71, 88,
	211, 60, 252, 40, 194, 106, 228, 217, 205, 118, 245, 22, 118, 249, 22, 165, 68, 101, 93, 174, 133, 134, 174, 165,
	226, 188, 230, 95, 102, 239, 169, 33, 23, 141, 42, 180, 103, 97, 169, 203, 229, 255, 2, 161, 51, 117, 221, 111, 82,
	0, 0
};
