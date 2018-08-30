
// マップエディット関連
function MapData(){
	this.terrainno = new Array();
	this.chipno = new Array();
	this.x = 0;
	this.y = 0;
	this.terrainname = new Array();
	this.selterr = -1;
	this.selchip = -1;

	this.init = function(x, y, dat, tnum, name){
    	this.x = x;
    	this.y = y;
		for(i=0; i<x*y; i++){
			this.terrainno[i] = dat[i*2];
			this.chipno[i] = dat[i*2]+1;
		}
		for(i=0; i<tnum; i++){
			this.terrainname[i] = name[i];
		}
	}
	this.reinit = function(newx, newy){
	    var ttmp = new Array();
	    var ctmp = new Array();
	    for(i=0; i<this.x*this.y; i++){
	        ttmp[i] = this.terrainno[i];
	        ctmp[i] = this.chipno[i];
	    }
		for(i=0; i<newx*newy; i++){
			this.terrainno[i] = 0;
			this.chipno[i] = 0;
		}
		for(i=0; i<this.x; i++){
		    for(j=0; j<this.y; j++){
    		    this.terrainno[newy*i+j] = ttmp[this.y*i+j];
	    	    this.chipno[newy*i+j] = ctmp[this.y*i+j];
			}
		}
    	this.x = newx;
    	this.y = newy;
	}
	this.changeTerrain = function(x, y){
//	document.editform.mapfile.value = "sz" + this.x + "," + this.y + ",set" + x + "," + y + "sel" + this.selterr +","+ this.selchip;
	    if(this.selterr < 0 || this.selchip < 0) return;
		this.terrainno[this.y*x+y] = this.selterr;
		this.chipno[this.y*x+y] = this.selchip;
	}
	this.getData = function(){
		var str = "\"MapData\"\n\"Reserved\"\n";
		str += "" + this.x + "," + this.y + "\n";
		for(i=0; i<this.x*this.y; i++){
			str += "" + this.terrainno[i] + "," + this.chipno[i] + "\n";
		}
		return str;
	}
	this.getFileName = function(terrain, chipno){
	    if(terrain < 0) terrain = 0;
	    if(chipno < 0) chipno = 0;
		return ("./mapchip/" + this.terrainname[terrain] + "/" + this.terrainname[terrain] + zerosapuresu(chipno, 4, 0) + ".bmp");
	}
	this.print = function(){
	    var type = 's';
	    var cols = '';
	    var str = "<table id=map border=1 cellspacing=0 cellpadding=0>\n";
		for(i=0; i<this.y; i++){
			if(type != 's' && i % 2){
				str += "\t<tr>\n\t\t<td class=map_td_h></td>\n";
			}else{
				str += "\t<tr>\n";
			}
			for(j=0; j<this.x; j++){
				str += "\t\t<td class=map_td" + cols + " id="+j+"_"+i+" style=\"background-image: url("+ this.getFileName(this.terrainno[this.y*j+i], this.chipno[this.y*j+i]) +");\" onmouseover=\"mapOver("+j+","+i+");\" onmousedown=\"mapClick("+j+","+i+");\">("+j+","+i+")</td>\n";
			}
			if(type != 's' && !(i % 2)){
				str += "\t\t<td class=map_td_h></td>\n\t</tr>\n";
			}else{
				str += "\t</tr>\n";
			}
		}
				
		str += "</table>\n";
	    document.getElementById("mainmap").innerHTML = str;
	}
}

function changeTerrain(){
	document.editform.mapfile.value = mapdat.getData();
}

function mapOver(x, y){
//	document.editform.mapfile.value = "Over:" + x + "," + y;//debug
	if(tmpunit != -82) return true;
	mapdat.changeTerrain(x,y);
    document.getElementById("" + x + "_" + y).style.backgroundImage = "url(" + mapdat.getFileName(mapdat.selterr, mapdat.selchip) + ")";
    return false;
}

function mapClick(x, y){
//	document.editform.mapfile.value = "Click" + x + "," + y;//debug
	mapdat.changeTerrain(x,y);
	tmpunit = -82;
    document.getElementById("" + x + "_" + y).style.backgroundImage = "url(" + mapdat.getFileName(mapdat.selterr, mapdat.selchip) + ")";
    return false;
}

function mapTerrainChange(){
    var n = document.mapedittool.terrain.selectedIndex
	mapdat.selterr = document.mapedittool.terrain.options[n].value;
//	document.editform.mapfile.value = "TerrainCg" + document.mapedittool.terrain.options[n].value;//debug
    for(i=0; i<64; i++){
        document.images["chip_" + i].src = mapdat.getFileName(mapdat.selterr, i);
    }
    return false;
}

function mapSelChipChange(chipno){
	mapdat.selchip = chipno;
    document.images["selmapchip"].src = mapdat.getFileName(mapdat.selterr, chipno);
    return false;
}

function changeMapType(type){
    var setx, sety, setchip;
    setx = parseInt(document.editform.mapw.value);
    sety = parseInt(document.editform.maph.value);
    setchip = parseInt(document.editform.mapchipsize.value);
    if(setx < 0) setx = 0;
    if(sety < 0) sety = 0;
    if(setx > 50) setx = 50;
    if(sety > 50) sety = 50;
    if(setchip < 4) setchip = 4;
    if(setx > 96) setchip = 96;
    if(type == 'd'){
        mapdat.reinit(setx, sety);
        mapdat.print();
    }else{
        printMapImg(setx,sety,setchip)
    }
//    alert(type);
}

function printMapImg(x,y,chip){
    var i, j;
    var type = 's';
    var cols = '';
	var str = "<table id=map border=1 cellspacing=0 cellpadding=0 background=";
	str += document.editform.mapfile.value + ">\n";
	for(i=0; i<y; i++){
		if(type != 's' && i % 2){
			str += "\t<tr>\n\t\t<td class=map_td_h></td>\n";
		}else{
			str += "\t<tr>\n";
		}
		for(j=0; j<x; j++){
			str += "\t\t<td class=map_td" + cols + ">("+i+","+j+")</td>\n";
		}
		if(type != 's' && !(i % 2)){
			str += "\t\t<td class=map_td_h></td>\n\t</tr>\n";
		}else{
			str += "\t</tr>\n";
		}
	}
	str += "</table>\n";
	document.getElementById("mainmap").innerHTML = str;
}

mapdat = new MapData();


