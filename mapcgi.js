

function zerosapuresu(num, keta, atama){
    var str = "" + num;
    while(str.length < keta){
        str = "" + atama + str;
    }
    return str;
}


function eveScroll(){
//	alert("Scroll");
//	var form = document.getElementById("floatform");
//	form.style.top = ;
//	form.style.left = ;
}

tmpunit = -1;   // マウスのクリック関連
prex = -1;
prey = -1;

if(navigator.appName.charAt(0) == 'N'){
	nnflag = true;
}else{
	nnflag = false;
}

window.document.onmousemove = eveUnitMove;
window.document.onmouseup = eveMouseup;

function eveUnitClick(uid){
//	alert("" + event.clientX + "," + event.clientY);
	tmpunit = uid;
//	if(nnflag){
//	}else{
		prex = event.clientX;
		prey = event.clientY;
//	}
	var obj;
	document.setting.unitid.value = uid;	// ＩＤ
	obj = document.images["ui_" + uid];		// 画像
	document.setting.unitfile.value = "" + obj.src;
	obj = document.getElementById("un_" + uid);	// 名前
	document.setting.unitname.value = "" + obj.innerText;
	obj = document.getElementById("ut_" + uid);	// テキスト
	document.setting.unittext.value = "" + obj.innerText;
	obj = document.getElementById("u_" + tmpunit);// 座標
	var tmp;
	tmp = "" + obj.style.top;
	tmp = tmp.substr(0, tmp.length - 2);
	document.setting.ay.value = tmp;
	tmp = "" + obj.style.left;
	tmp = tmp.substr(0, tmp.length - 2);
	document.setting.ax.value = tmp;
	eveImgChange();
	return false;
}

function eveImgChange(){
	document.images["imgpreview"].src = document.setting.unitfile.value;
	return true;
}

function eveFormClick(){
	tmpunit = -40;
	prex = event.clientX;
	prey = event.clientY;
}

function eveMouseup(){
	if(tmpunit == -82) changeTerrain();
	tmpunit = -1;
}

// ユニットムーブとなっているが、事実上マウスムーブ
function eveUnitMove(){
	if(tmpunit > 0){
		var cmpx, cmpy;
		cmpx = event.clientX - prex;
		cmpy = event.clientY - prey;
		var obj = document.getElementById("u_" + tmpunit);
		var tmp;
		tmp = "" + obj.style.top;
		tmp = tmp.substr(0, tmp.length - 2);
		obj.style.top = "" + (tmp * 1 + + cmpy) + "px";
		document.setting.ay.value = (tmp * 1 + cmpy);
		tmp = "" + obj.style.left;
		tmp = tmp.substr(0, tmp.length - 2);
		obj.style.left = "" + (tmp * 1 + cmpx) + "px";
		document.setting.ax.value = (tmp * 1 + cmpx);
		prex = event.clientX;
		prey = event.clientY;
		return false;
	}else if(tmpunit == -40){
		var cmpx, cmpy;
		cmpx = event.clientX - prex;
		cmpy = event.clientY - prey;
		var obj = document.getElementById("floatform");
		var tmp;
		tmp = "" + obj.style.top;
		tmp = tmp.substr(0, tmp.length - 2);
		obj.style.top = "" + (tmp * 1 + + cmpy) + "px";
		tmp = "" + obj.style.left;
		tmp = tmp.substr(0, tmp.length - 2);
		obj.style.left = "" + (tmp * 1 + + cmpx) + "px";
		prex = event.clientX;
		prey = event.clientY;
		if(document.setting){
    		document.setting.formx.value = "" + obj.style.left;
    		document.setting.formy.value = "" + obj.style.top;
    	}
    	if(document.editform){
    		document.editform.formx.value = "" + obj.style.left;
    		document.editform.formy.value = "" + obj.style.top;
    	}
		return false;
	}
	return true;
}

function unitActive(uid){
	var obj = document.getElementById("u_" + uid);
	obj.style.zIndex = 16;
	obj = document.getElementById("un_" + uid);
	obj.style.visibility = "visible";
	obj = document.getElementById("ut_" + uid);
	obj.style.visibility = "visible";
	return false;
}

function unitFree(uid){
	var obj = document.getElementById("u_" + uid);
	obj.style.zIndex = 11;
	obj = document.getElementById("un_" + uid);
	obj.style.visibility = "hidden";
	obj = document.getElementById("ut_" + uid);
	obj.style.visibility = "hidden";
	return false;
}
