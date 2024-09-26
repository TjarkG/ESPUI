const char JS_GRAPH[] PROGMEM = R"=====(
function lineGraph(parent,xAccessor,yAccessor){const width=620;const height=420;const gutter=40;const pixelsPerTick=30;function numericTransformer(dataMin,dataMax,pxMin,pxMax){var dataDiff=dataMax-dataMin,pxDiff=pxMax-pxMin,dataRatio=pxDiff/dataDiff,coordRatio=dataDiff/pxDiff;return{toCoord:function(data){return(data-dataMin)*dataRatio+pxMin;},toData:function(coord){return(coord-pxMin)*coordRatio+dataMin;}};}
function axisRenderer(orientation,transform){var axisGroup=document.createElementNS("http://www.w3.org/2000/svg","g");var axisPath=document.createElementNS("http://www.w3.org/2000/svg","path");axisGroup.setAttribute("class",orientation+"-axis");var xMin=gutter;var xMax=width-gutter;var yMin=height-gutter;var yMax=gutter;if(orientation==="x"){axisPath.setAttribute("d","M "+xMin+" "+yMin+" L "+xMax+" "+yMin);for(var i=xMin;i<=xMax;i++){if((i-xMin)%(pixelsPerTick*3)===0&&i!==xMin){var text=document.createElementNS("http://www.w3.org/2000/svg","text");text.innerHTML=new Date(Math.floor(transform(i))).toLocaleTimeString();text.setAttribute("x",i);text.setAttribute("y",yMin);text.setAttribute("dy","1em");axisGroup.appendChild(text);}}}else{axisPath.setAttribute("d","M "+xMin+" "+yMin+" L "+xMin+" "+yMax);for(var i=yMax;i<=yMin;i++){if((i-yMin)%pixelsPerTick===0&&i!==yMin){var tickGroup=document.createElementNS("http://www.w3.org/2000/svg","g");var gridLine=document.createElementNS("http://www.w3.org/2000/svg","path");text=document.createElementNS("http://www.w3.org/2000/svg","text");text.innerHTML=Math.floor(transform(i));text.setAttribute("x",xMin);text.setAttribute("y",i);text.setAttribute("dx","-.5em");text.setAttribute("dy",".3em");gridLine.setAttribute("d","M "+xMin+" "+i+" L "+xMax+" "+i);tickGroup.appendChild(gridLine);tickGroup.appendChild(text);axisGroup.appendChild(tickGroup);}}}
axisGroup.appendChild(axisPath);parent.appendChild(axisGroup);}
function lineRenderer(xAccessor,yAccessor,xTransform,yTransform){var line=document.createElementNS("http://www.w3.org/2000/svg","path");xAccessor.reset();yAccessor.reset();if(!xAccessor.hasNext()||!yAccessor.hasNext()){return;}
var pathString="M "+xTransform(xAccessor.next())+" "+yTransform(yAccessor.next());while(xAccessor.hasNext()&&yAccessor.hasNext()){pathString+=" L "+
xTransform(xAccessor.next())+
" "+
yTransform(yAccessor.next());}
line.setAttribute("class","series");line.setAttribute("d",pathString);parent.appendChild(line);}
function pointRenderer(xAccessor,yAccessor,xTransform,yTransform){var pointGroup=document.createElementNS("http://www.w3.org/2000/svg","g");pointGroup.setAttribute("class","data-points");xAccessor.reset();yAccessor.reset();if(!xAccessor.hasNext()||!yAccessor.hasNext()){return;}
while(xAccessor.hasNext()&&yAccessor.hasNext()){var xDataValue=xAccessor.next();var x=xTransform(xDataValue);var yDataValue=yAccessor.next();var y=yTransform(yDataValue);var circle=document.createElementNS("http://www.w3.org/2000/svg","circle");circle.setAttribute("cx",x);circle.setAttribute("cy",y);circle.setAttribute("r","4");var text=document.createElementNS("http://www.w3.org/2000/svg","text");text.innerHTML=Math.floor(xDataValue)+" / "+Math.floor(yDataValue);text.setAttribute("x",x);text.setAttribute("y",y);text.setAttribute("dx","1em");text.setAttribute("dy","-.7em");pointGroup.appendChild(circle);pointGroup.appendChild(text);}
parent.appendChild(pointGroup);}
xTransform=numericTransformer(xAccessor.min(),xAccessor.max(),0+gutter,width-gutter);yTransform=numericTransformer(yAccessor.min(),yAccessor.max(),height-gutter,0+gutter);axisRenderer("x",xTransform.toData);axisRenderer("y",yTransform.toData);lineRenderer(xAccessor,yAccessor,xTransform.toCoord,yTransform.toCoord);pointRenderer(xAccessor,yAccessor,xTransform.toCoord,yTransform.toCoord);}
function renderGraphSvg(dataArray,renderId){var figure=document.getElementById(renderId);while(figure.hasChildNodes()){figure.removeChild(figure.lastChild);}
var svg=document.createElementNS("http://www.w3.org/2000/svg","svg");svg.setAttribute("viewBox","0 0 640 440");svg.setAttribute("preserveAspectRatio","xMidYMid meet");lineGraph(svg,(function(data,min,max){var i=0;return{hasNext:function(){return i<data.length;},next:function(){return data[i++].x;},reset:function(){i=0;},min:function(){return min;},max:function(){return max;}};})(dataArray,Math.min.apply(Math,dataArray.map(function(o){return o.x;})),Math.max.apply(Math,dataArray.map(function(o){return o.x;}))),(function(data,min,max){var i=0;return{hasNext:function(){return i<data.length;},next:function(){return data[i++].y;},reset:function(){i=0;},min:function(){return min;},max:function(){return max;}};})(dataArray,Math.min.apply(Math,dataArray.map(function(o){return o.y;})),Math.max.apply(Math,dataArray.map(function(o){return o.y;}))));figure.appendChild(svg);}
)=====";

const uint8_t JS_GRAPH_GZIP[1280] PROGMEM = {
	31, 139, 8, 0, 3, 199, 199, 101, 2, 255, 205, 87, 95, 111, 219, 54, 16, 127, 247, 167, 112, 4, 52, 16, 99, 89, 86,
	27, 175, 3, 170, 240, 33, 109, 135, 174, 64, 18, 20, 77, 48, 96, 24, 246, 192, 73, 180, 76, 76, 150, 4, 138, 182,
	69, 184, 254, 238, 59, 146, 162, 36, 59, 82, 134, 58, 41, 182, 135, 56, 34, 239, 47, 239, 126, 119, 71, 46, 214, 89,
	36, 88, 158, 141, 83, 150, 209, 79, 156, 20, 75, 183, 32, 156, 102, 194, 171, 174, 163, 136, 150, 101, 206, 61, 105,
	191, 208, 46, 202, 179, 82, 140, 183, 44, 22, 75, 252, 246, 77, 16, 154, 245, 146, 178, 100, 41, 240, 188, 217, 72,
	214, 66, 80, 142, 231, 118, 93, 176, 138, 166, 229, 23, 202, 31, 88, 244, 55, 190, 12, 194, 133, 53, 155, 173, 87,
	148, 179, 232, 129, 147, 172, 92, 228, 28, 22, 110, 76, 4, 185, 101, 153, 167, 255, 147, 202, 43, 42, 181, 130, 95,
	82, 161, 221, 134, 240, 177, 34, 124, 100, 139, 5, 174, 57, 166, 86, 162, 168, 244, 182, 102, 157, 26, 49, 69, 250,
	74, 192, 20, 54, 196, 153, 21, 246, 162, 60, 231, 177, 33, 217, 189, 153, 225, 9, 57, 21, 107, 158, 237, 68, 254,
	65, 241, 188, 179, 206, 106, 207, 208, 206, 80, 245, 194, 90, 70, 23, 141, 157, 137, 182, 27, 238, 61, 145, 127,
	132, 189, 86, 88, 219, 107, 164, 245, 202, 248, 136, 46, 90, 87, 38, 181, 194, 112, 191, 15, 247, 163, 38, 74, 164,
	98, 229, 87, 154, 197, 148, 67, 124, 114, 206, 32, 63, 138, 59, 243, 132, 141, 155, 137, 140, 226, 251, 196, 243,
	117, 129, 227, 60, 130, 200, 102, 194, 143, 56, 37, 130, 254, 146, 82, 181, 186, 187, 119, 157, 165, 16, 197, 187,
	217, 108, 187, 221, 250, 219, 75, 63, 231, 201, 236, 77, 16, 4, 179, 114, 147, 56, 158, 147, 56, 40, 180, 138, 190,
	16, 72, 242, 137, 122, 10, 144, 5, 85, 141, 63, 126, 73, 197, 181, 16, 156, 253, 181, 22, 212, 117, 162, 148, 148,
	165, 227, 117, 78, 50, 113, 166, 138, 185, 54, 175, 194, 130, 13, 136, 234, 53, 169, 176, 70, 221, 180, 179, 43, 21,
	151, 193, 222, 225, 54, 48, 215, 107, 182, 232, 134, 11, 99, 236, 84, 14, 218, 217, 211, 29, 121, 21, 131, 227, 183,
	99, 103, 162, 172, 79, 28, 248, 144, 230, 227, 70, 239, 145, 170, 217, 67, 33, 132, 220, 85, 182, 24, 214, 217, 102,
	87, 88, 49, 132, 108, 50, 65, 59, 176, 233, 178, 169, 206, 236, 43, 247, 0, 249, 23, 151, 8, 92, 8, 206, 207, 217,
	25, 214, 130, 38, 107, 130, 86, 226, 212, 64, 43, 89, 8, 154, 250, 231, 179, 44, 163, 252, 215, 135, 219, 27, 156,
	209, 237, 24, 208, 71, 221, 91, 117, 202, 69, 10, 248, 114, 27, 168, 184, 12, 33, 228, 139, 252, 38, 143, 72, 74,
	31, 216, 138, 222, 67, 8, 178, 196, 173, 213, 28, 6, 165, 114, 60, 214, 75, 144, 142, 103, 98, 209, 67, 139, 129,
	232, 188, 166, 171, 3, 8, 144, 162, 0, 8, 127, 88, 178, 52, 118, 149, 12, 2, 144, 239, 33, 56, 244, 180, 124, 52,
	123, 208, 22, 58, 249, 144, 58, 15, 87, 88, 234, 188, 180, 249, 208, 190, 190, 58, 108, 68, 77, 46, 100, 155, 11,
	216, 127, 145, 10, 74, 56, 139, 111, 160, 167, 62, 179, 130, 94, 30, 27, 67, 144, 24, 72, 126, 53, 148, 99, 57, 4,
	140, 24, 164, 156, 169, 255, 147, 78, 255, 16, 56, 252, 75, 77, 182, 81, 250, 183, 204, 179, 227, 50, 84, 166, 109,
	170, 14, 144, 101, 53, 14, 209, 13, 242, 6, 80, 105, 37, 52, 52, 71, 253, 76, 22, 172, 40, 52, 131, 242, 17, 209,
	106, 104, 219, 183, 154, 173, 77, 251, 238, 153, 171, 94, 213, 204, 63, 79, 62, 28, 182, 244, 244, 249, 24, 106, 44,
	250, 156, 66, 156, 161, 206, 229, 163, 29, 40, 146, 179, 150, 111, 73, 202, 59, 136, 147, 139, 190, 125, 59, 147,
	143, 119, 237, 20, 131, 51, 42, 23, 149, 25, 211, 66, 176, 201, 90, 115, 132, 246, 176, 126, 102, 36, 77, 205, 182,
	12, 242, 152, 33, 220, 66, 32, 169, 219, 227, 203, 249, 121, 175, 43, 173, 245, 9, 54, 32, 25, 61, 233, 192, 72,
	121, 48, 122, 210, 133, 253, 40, 125, 140, 201, 122, 102, 57, 37, 220, 87, 168, 26, 84, 105, 47, 110, 91, 119, 122,
	1, 146, 106, 104, 118, 176, 81, 228, 44, 19, 167, 130, 67, 11, 63, 187, 93, 181, 90, 6, 142, 172, 175, 58, 154, 171,
	252, 193, 112, 250, 222, 228, 235, 203, 129, 186, 103, 253, 70, 210, 53, 197, 199, 217, 54, 151, 7, 220, 197, 67,
	195, 108, 136, 178, 21, 150, 125, 194, 18, 119, 145, 114, 36, 28, 49, 30, 165, 39, 87, 167, 145, 134, 128, 154, 143,
	227, 216, 171, 222, 59, 68, 83, 179, 119, 128, 198, 65, 243, 188, 158, 65, 63, 116, 120, 116, 34, 9, 69, 61, 131,
	162, 234, 16, 187, 145, 26, 152, 43, 131, 151, 138, 225, 161, 242, 250, 201, 145, 50, 245, 127, 214, 244, 14, 156,
	187, 149, 103, 162, 53, 72, 174, 111, 36, 163, 158, 154, 109, 37, 20, 67, 11, 38, 220, 243, 120, 105, 17, 184, 98,
	153, 139, 188, 206, 154, 84, 176, 14, 38, 230, 102, 234, 117, 111, 179, 80, 65, 79, 42, 149, 71, 74, 229, 145, 210,
	131, 59, 112, 99, 194, 76, 185, 166, 181, 232, 160, 55, 90, 125, 243, 60, 57, 230, 81, 241, 127, 204, 243, 29, 243,
	203, 175, 31, 77, 135, 106, 244, 86, 29, 250, 151, 80, 212, 233, 159, 92, 171, 211, 47, 215, 251, 77, 162, 95, 101,
	215, 156, 19, 233, 25, 194, 231, 216, 52, 137, 5, 75, 214, 188, 83, 170, 9, 21, 117, 37, 188, 151, 159, 99, 183, 97,
	174, 199, 143, 97, 87, 157, 70, 67, 224, 46, 143, 105, 169, 250, 77, 189, 207, 233, 42, 223, 80, 131, 142, 122, 11,
	90, 165, 208, 27, 168, 158, 138, 80, 70, 167, 214, 158, 250, 69, 33, 252, 30, 193, 124, 195, 232, 246, 125, 174, 42,
	33, 24, 7, 227, 183, 243, 96, 60, 159, 7, 189, 156, 133, 106, 196, 124, 67, 175, 203, 130, 70, 66, 63, 45, 65, 10,
	174, 83, 241, 239, 240, 55, 94, 81, 42, 234, 9, 102, 222, 252, 160, 193, 115, 15, 222, 185, 30, 64, 205, 91, 217,
	39, 55, 195, 129, 125, 21, 215, 221, 183, 125, 216, 218, 246, 61, 102, 87, 74, 208, 79, 105, 150, 136, 37, 188, 128,
	179, 126, 54, 197, 244, 7, 220, 204, 255, 244, 43, 96, 210, 19, 163, 203, 165, 76, 237, 149, 245, 30, 209, 149, 126,
	89, 131, 87, 125, 52, 184, 247, 171, 87, 51, 234, 96, 64, 119, 36, 16, 82, 229, 156, 74, 253, 26, 242, 26, 42, 20,
	79, 209, 158, 57, 111, 20, 229, 202, 47, 132, 106, 97, 82, 157, 34, 140, 254, 131, 104, 202, 255, 105, 52, 229, 115,
	162, 169, 133, 225, 74, 86, 87, 89, 183, 45, 3, 102, 161, 214, 254, 1, 223, 145, 42, 131, 193, 18, 0, 0
};
