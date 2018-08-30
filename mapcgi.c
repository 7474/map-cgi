
/**********************************************/
/* ヘッダ部 ***********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>

#include "libcgi.h"
//#include "libpr.h"
//#include "libstr.h"


/* 設定 **************************************************/

/* メモリ確保単位 */
#define MALOC_SIZE (32)

#define UID "u_"
#define UIMG "ui_"
#define UNAME "un_"
#define UTXT "ut_"

const char ext[] = "bmp";
const char chipdir[] = "./mapchip/";
const char datdir[] = "./dat/";

#define EOID (-1)

/* 文字列長制限 */
#define PASSMAX (16)
#define NAMEMAX (32)
#define FILEMAX (32)
#define URLMAX (256)
#define UNITTEXTMAX (512)

#define JS_NAME "mapcgi.js"

/* 構造体 **********************************************/
/* メモリ管理面倒なので文字列は定数長 */

typedef struct MapChip{
	int terrain, num;
}MapChip;

typedef struct TerrainData{
	char name[NAMEMAX];
	char file[FILEMAX];
	char type[5];
	int cost, hit, avd;
}TerrainData;

typedef struct MapData{
	int id;// id=0:未定義
	int x, y;
	int cw, ch;
	MapChip *dat;
	TerrainData *tdat;
	char mapimg[URLMAX];
	char maptype, masutype;
}MapData;

typedef struct UnitData{
	int id, oid;// oid=所有者 id=-1:終端
	int view;
	int mx, my, ax, ay;
	int cw, ch;
	char file[URLMAX], name[NAMEMAX], text[UNITTEXTMAX];
}UnitData;

typedef struct UserData{
	int id;// id=0:未定義 id=-1:終端
	char type;// m=マスター p=プレイヤ
	char name[NAMEMAX], password[PASSMAX+1];
}UserData;

typedef struct SessionData{
	char datfile[FILEMAX], mapfile[URLMAX], title[NAMEMAX], password[PASSMAX+1];
	MapData *mapdat;
	UnitData *unitdat;
	UserData *userdat;
	int useruse, unituse, usermem, unitmem, nextuserid, nextunitid;
	char maptype, masutype;
}SessionData;


/* プロトタイプ宣言 *****************************************/

/* MapData **************************************************/
void MapDataFree(MapData *dat);
MapData *MapDataRead(FILE *fp, char maptype, char masutype);
int MapDataPrint(MapData *dat, FILE *fp);
int MapDataPrintEdit(MapData *dat, FILE *fp);
int MapDataPrintEditForm(MapData *dat, FILE *fp);

TerrainData *TerrainDataCreate(FILE *fp);

MapChip *MapChipSetData(MapChip *dat, int terrain, int num);
char *MapChipGetFileName(MapChip *dat, TerrainData *tdat);

/* UnitData **************************************************/
void UnitDataFree(UnitData *dat);
UnitData *UnitDataCopy(UnitData *dat, UnitData *sdat);
UnitData *UnitDataSet(UnitData *dat, int id, int oid, char *file, char *name, char *text, int ax, int ay);
UnitData *UnitDataSetPos(UnitData *dat, int ax, int ay);

UnitData *UnitDataRead(UnitData *dat, FILE *fp);
UnitData *UnitDataWrite(UnitData *dat, FILE *fp);

int UnitDataPrint(UnitData *dat, FILE *fp);

/* UserData **************************************************/
void *UserDataFree(UserData *dat);
UserData *UserDataCopy(UserData *dat, UserData *sdat);
UserData *UserDataSet(UserData *dat, int id, char type, char *name, char *password);

UserData *UserDataRead(UserData *dat, FILE *fp);
UserData *UserDataWrite(UserData *dat, FILE *fp);

/* SessionData ***********************************************/
void SessionDataFree(SessionData *dat);
SessionData *SessionDataMemInit(SessionData *dat);
SessionData *SessionDataCreate(char *datfile, char *mapfile, char maptype, char masutype, char *title, char *username, char *password);
SessionData *SessionDataRead(FILE *fp);
SessionData *SessionDataReadFile(char *datfile);
int SessionDataWrite(SessionData *dat, FILE *fp);
int SessionDataWriteFile(SessionData *dat);

SessionData *SessionDataDelUser(SessionData *dat, int delid);
SessionData *SessionDataAddUser(SessionData *dat, UserData *adddat);
SessionData *SessionDataDelUnit(SessionData *dat, int delid, UserData *userdat);
SessionData *SessionDataAddUnit(SessionData *dat, UnitData *adddat, UserData *userdat);

UserData *GetUserDataID(SessionData *dat, int userid, char *password);
UserData *GetUserDataName(SessionData *dat, char *username, char *password);

UnitData *GetUnitDataID(SessionData *dat, int unitid);
UnitData *GetUnitDataName(SessionData *dat, char *unitname);


/* output *****************************************************/
void HeaderPrint(FILE *fp, char *title);
void FooterPrint(FILE *fp);
void ErrorPrint(FILE *fp, char *str);

void PrintMainForm(FILE *fp, char *pass);/* ログイン等 */

void PrintNewSessionForm(FILE *fp, char *pass);/* 新規作成フォーム（ログイン合わせ？ */
void PrintEditForm(FILE *fp, char *pass, SessionData *dat, UserData *userdat, char *formx, char *formy);/* マップ編集フォーム（後で */
void PrintSessionView(FILE *fp, char *pass, SessionData *dat, UserData *userdat, char *formx, char *formy);/* メイン画面（ユニット移動含む */

void PrintDataList(FILE *fp, char *pass);

/* debug ******************************************************/
void PrintMap(FILE *fp, char *mapfile);

/* string *****************************************************/
char *KillSpace(char *str);

FILE *flopen(char *file, char *mode);


/***********************************************/
/* 実体 ************************************/

int main(int argc, char *argv[]){
	MapData *dat;
	FILE *fp;

	int		i;							/*カウンタ*/
	char	buffer[BUFSIZE];			/*バッファ*/
	char	*method;					/*REQUEST_METHOD*/
	int		count;						/*フォームデータname=valueの組数*/
	char	**name, **value;			/*フォームデータ取得*/
	char	*temp;

/* formdata */
	char *datfile;
	char *mapfile;
	char *mapname;
	char *password;
	char *mode;
	char *userid, *username;
	char *unitid, *unitfile, *unitname, *unittext, *ax, *ay;
	char *newunit;
	char *maptype, *masutype, *mapw, *maph, *mapchipsize;
	char *formx, *formy;

	char mtmp;	/* mode switch */
/* object */
	SessionData *sessiondat;
	UserData userdat;
	UnitData unitdat;

	char mapfilestr[URLMAX], datfilestr[URLMAX];

/* test */
	char dumyfilename[16];
	char *file;
	file = NULL;
	strcpy(dumyfilename, "null.png");

	datfile = mapfile = mapname = password = 
	userid = unitfile = username = unitid = unitname = ax = ay =
	newunit =
	formx = formy =
	maptype = masutype = mapw =maph = mapchipsize =
	mode = method = NULL;

	sessiondat = NULL;

	method = getenv("REQUEST_METHOD");
	if(method == NULL) method = "Unknown";

	/*----- フォーム解析 -----*/
	count = getForm(&name, &value);	/*POSTデータ取得*/
	for(i = 0; i < count; i++){
		if(!strcmp(*(name + i),"datfile")){
			datfile = *(value + i);
		}else if(!strcmp(*(name + i),"mapfile")){
			mapfile = *(value + i);
		}else if(!strcmp(*(name + i),"mapname")){
			mapname = *(value + i);
		}else if(!strcmp(*(name + i),"password")){
			password = *(value + i);
		}else if(!strcmp(*(name + i),"userid")){
			userid = *(value + i);
		}else if(!strcmp(*(name + i),"username")){
			username = *(value + i);
		}else if(!strcmp(*(name + i),"unitid")){
			unitid = *(value + i);
		}else if(!strcmp(*(name + i),"unitfile")){
			unitfile = *(value + i);
		}else if(!strcmp(*(name + i),"unitname")){
			unitname = *(value + i);
		}else if(!strcmp(*(name + i),"unittext")){
			unittext = *(value + i);
		}else if(!strcmp(*(name + i),"ax")){
			ax = *(value + i);
		}else if(!strcmp(*(name + i),"ay")){
			ay = *(value + i);
		}else if(!strcmp(*(name + i),"newunit")){
			newunit = *(value + i);
		}else if(!strcmp(*(name + i),"formx")){
			formx = *(value + i);
		}else if(!strcmp(*(name + i),"formy")){
			formy = *(value + i);
		}else if(!strcmp(*(name + i),"maptype")){
			maptype = *(value + i);
		}else if(!strcmp(*(name + i),"masutype")){
			masutype = *(value + i);
		}else if(!strcmp(*(name + i),"mapw")){
			mapw = *(value + i);
		}else if(!strcmp(*(name + i),"maph")){
			maph = *(value + i);
		}else if(!strcmp(*(name + i),"mapchipsize")){
			mapchipsize = *(value + i);
		}else if(!strcmp(*(name + i),"mode")){
			mode = *(value + i);
		}else if(!strcmp(*(name + i),"file")){
			file = *(value + i);
		}
	}

	fprintf(stdout, "Content-type: text/html;\n\n");

	if(newunit != NULL){
		if(!strcmp(newunit, "new")) unitid = "-1";
	}
	if(file != NULL && strstr(file, "../")) file = NULL;
	if(mapfile != NULL && strstr(mapfile, "../")) file = NULL;
	if(datfile != NULL && strstr(datfile, "../")) file = NULL;
	if(unitfile == NULL || strlen(unitfile) == 0) unitfile = dumyfilename;
	/* モード分岐 */
	if(mode){ mtmp = *mode;
	}else{ mtmp = 0;}
	switch(mtmp){
	/* 新規マップの追加処理 ****************************************/
	case 'c':
		if(!(mapfile && mapname && username)) break;
		if(strlen(mapname) == 0) break;
		if(strlen(username) == 0) break;
		if(masutype == NULL) masutype = "sq";
		if(maptype == NULL) maptype = "dat";
		sprintf(mapfilestr, "map_%d.map", datdir, time(NULL));
		sprintf(datfilestr, "%s%s", datdir, mapfilestr);
		if(fp = flopen(datfilestr, "w")){
			if(*maptype == 'd'){
				fprintf(fp, "%s", mapfile);
			}else{
				fprintf(fp, "%d,%d,%d\n", atoi(mapw), atoi(maph), atoi(mapchipsize));
				fprintf(fp, "%s", mapfile);
			}
			fclose(fp);
			sprintf(datfilestr, "dat_%s.dat", mapname);
			sessiondat = SessionDataCreate(datfilestr, mapfilestr, *maptype, *masutype, mapname, username, password);
			if(sessiondat){
				SessionDataWriteFile(sessiondat);
				PrintSessionView(stdout, argv[0], sessiondat, GetUserDataID(sessiondat, 1, password), formx, formy);
			}else{
				PrintMainForm(stdout, argv[0]);
			}
		}else{
			PrintMainForm(stdout, argv[0]);
		}
		break;
	/* マップの編集処理（フォーム表示とコミット？ *******************************/
	case 'e':
		sprintf(datfilestr, "%s%s", datdir, datfile);
		sessiondat = SessionDataReadFile(datfilestr);
		if(sessiondat){
			PrintEditForm(stdout, argv[0], sessiondat, GetUserDataID(sessiondat, atoi(userid), password), formx, formy);
		}else{
			PrintMainForm(stdout, argv[0]);
		}
		break;
	/* マップの編集データの受け取り処理 *******************************/
	case 'r':
		sprintf(datfilestr, "%s%s", datdir, datfile);
		sessiondat = SessionDataReadFile(datfilestr);
		if(sessiondat){
			if(GetUserDataID(sessiondat, atoi(userid), password) == NULL) break;
			if(GetUserDataID(sessiondat, atoi(userid), password)->type != 'm') break;
			if(masutype == NULL) masutype = "sq";
			if(maptype == NULL) maptype = "dat";
			sprintf(datfilestr, "%s%s", datdir, sessiondat->mapfile);
			if(fp = flopen(datfilestr, "w")){
				if(*maptype == 'd'){
					fprintf(fp, "%s", mapfile);
				}else{
					fprintf(fp, "%d,%d,%d\n", atoi(mapw), atoi(maph), atoi(mapchipsize));
					fprintf(fp, "%s", mapfile);
				}
				fclose(fp);
				sessiondat->maptype = *maptype;
				sessiondat->masutype = *masutype;
				SessionDataWriteFile(sessiondat);
			}else{
				break;
			}
			SessionDataFree(sessiondat);
			sprintf(datfilestr, "%s%s", datdir, datfile);
			sessiondat = SessionDataReadFile(datfilestr);
//sessiondat = NULL;
			if(sessiondat){
				PrintEditForm(stdout, argv[0], sessiondat, GetUserDataID(sessiondat, atoi(userid), password), formx, formy);
			}else{
				PrintMainForm(stdout, argv[0]);
			}
		}else{
			PrintMainForm(stdout, argv[0]);
		}
		break;
	/* ユーザーログイン処理 *******************************/
	case 'l':
		sprintf(datfilestr, "%s%s", datdir, datfile);
		sessiondat = SessionDataReadFile(datfilestr);
		if(sessiondat){
			if(strlen(username)){
				SessionDataAddUser(sessiondat, UserDataSet(&userdat, EOID, 'u', username, password));
				SessionDataWriteFile(sessiondat);
			}
			PrintSessionView(stdout, argv[0], sessiondat, GetUserDataName(sessiondat, username, password), formx, formy);
		}else{
			PrintMainForm(stdout, argv[0]);
		}
		break;
	/* ユニット追加処理 *******************************/
	case 'a':
		sprintf(datfilestr, "%s%s", datdir, datfile);
		sessiondat = SessionDataReadFile(datfilestr);
		if(sessiondat){
			if(atoi(unitid) > 0){
				if(strlen(unitname)){
					SessionDataAddUnit(sessiondat,
						UnitDataSet(&unitdat, atoi(unitid), atoi(userid), unitfile, unitname, unittext, atoi(ax), atoi(ay)),
						GetUserDataID(sessiondat, atoi(userid), password));
					SessionDataWriteFile(sessiondat);
				}else{// 名前無しで削除
					SessionDataDelUnit(sessiondat, atoi(unitid), GetUserDataID(sessiondat, atoi(userid), password));
					SessionDataWriteFile(sessiondat);
				}
			}else if(strlen(unitname)){//ユニットID無しなら追加
				SessionDataAddUnit(sessiondat,
					UnitDataSet(&unitdat, atoi(unitid), atoi(userid), unitfile, unitname, unittext, atoi(ax), atoi(ay)),
					GetUserDataID(sessiondat, atoi(userid), password));
				SessionDataWriteFile(sessiondat);
			}
			PrintSessionView(stdout, argv[0], sessiondat, GetUserDataID(sessiondat, atoi(userid), password), formx, formy);
		}else{
			PrintMainForm(stdout, argv[0]);
		}
		break;
	/* マップの表示処理（ユニット追加と合わせ？ **********************************/
	case 'v':
		sprintf(datfilestr, "%s%s", datdir, datfile);
		sessiondat = SessionDataReadFile(datfilestr);
		if(sessiondat){
			if(userid == NULL) userid = "-1";
			PrintSessionView(stdout, argv[0], sessiondat, GetUserDataID(sessiondat, atoi(userid), password), formx, formy);
		}else{
			PrintMainForm(stdout, argv[0]);
		}
		break;
	default:
		if(file != NULL){
			PrintMap(stdout, file);
		}else{
			PrintMainForm(stdout, argv[0]);
		}
		break;
	}

	/* データを解放して終了 */
	if(sessiondat != NULL) SessionDataFree(sessiondat);
	freedata(name, value);
	exit(0);
}


/************************************************************/
/* MapData ***************************************************/

MapData *MapDataRead(FILE *fp, char maptype, char masutype){
	MapData *dat;
	FILE *tfp;
	int cnt, err;
	char buf[256];
	MapChip *chip;

	dat = malloc(sizeof(MapData));
	if(dat == NULL) return NULL;
	dat->tdat = NULL;
	dat->id = (int)dat;
	dat->x = 0;
	dat->y = 0;
	dat->cw = dat->ch = 32;
	dat->maptype = maptype;
	dat->masutype = masutype;
	dat->mapimg[0] = '\0';

	//fprintf(stderr, "MapData\n");
	while(fgets(buf, sizeof(buf), fp)){
		KillSpace(buf);
		if(maptype == 'd'){
			if(sscanf(buf, "%d,%d\n", &(dat->x), &(dat->y)) == 2)break;
		}else{
			if(sscanf(buf, "%d,%d,%d\n", &(dat->x), &(dat->y), &(dat->cw)) == 3){
				dat->ch = dat->cw;
				fscanf(fp, "%s", dat->mapimg);
//				return dat;
			}
		}
	}
	if(maptype == 'd'){
		//fprintf(stderr, "size:%d, %d\n", dat->x, dat->y);
		cnt = 0;
		dat->dat = malloc(sizeof(MapChip) * dat->x * dat->y);
		chip = dat->dat;
		while(cnt++ < dat->x*dat->y){
			if(fgets(buf, sizeof(buf), fp) == NULL){
				MapDataFree(dat);
				return NULL;
			}
			KillSpace(buf);
			if(sscanf(buf, "%d,%d\n", &(chip->terrain), &(chip->num)) != 2){
				MapDataFree(dat);
				return NULL;
			}
			//fprintf(stderr, "dat%d:%d, %d\n", cnt, chip->terrain, chip->num);
			chip++;
		}
	}
	if((tfp = flopen("terrain.txt", "r")) != NULL){
	//fprintf(stderr, "Terrain\n");
		dat->tdat = TerrainDataCreate(tfp);
		fclose(tfp);
		if(dat->tdat == NULL){
			free(dat);
			return NULL;
		}
	}else{
		return NULL;
	}

	return dat;
}

void MapDataFree(MapData *dat){
	if(dat->dat) free(dat->dat);
	if(dat->tdat) free(dat->tdat);
	free(dat);
}

#define GetMapChip(v,w) (chip+v+w*dat->y)

int MapDataPrint(MapData *dat, FILE *fp){
	int i, j;
	char *tdstr, *tdstr2;
	MapChip *chip;

	if(dat->masutype == 's'){
		tdstr = "";
	}else{
		tdstr = " colspan=2";
	}
	fprintf(fp, "<style type=text/css>\n"
		"<!--\ntd.map_td{\n"
		"width: %dpx; height: %dpx; font-size: xx-small;\n"
		"}\n"
		"td.map_td_h{\n"
		"width: %dpx; height: %dpx; font-size: xx-small;\n"
		"//-->\n</style>\n",
		dat->cw, dat->ch, dat->cw/2, dat->ch);
	if(dat->maptype == 'd'){
		chip = dat->dat;
		fprintf(fp, "<div id=mainmap style=\"position:absolute; top: 0; left: 0; z-index: 1;\">"
			"<table id=map border=1 cellspacing=0 cellpadding=0>\n");
		for(i=0; i<dat->y; i++){
			if(dat->masutype != 's' && i % 2){
				fprintf(fp, "\t<tr>\n\t\t<td class=map_td_h></td>\n");
			}else{
				fprintf(fp, "\t<tr>\n");
			}
			for(j=0; j<dat->x; j++){
				fprintf(fp, "\t\t<td class=map_td %s background=%s%s>(%d,%d)</td>\n", tdstr,
					chipdir, MapChipGetFileName(GetMapChip(i,j), dat->tdat), j, i);
			}
			if(dat->masutype != 's' && !(i % 2)){
				fprintf(fp, "\t\t<td class=map_td_h></td>\n\t</tr>\n");
			}else{
				fprintf(fp, "\t</tr>\n");
			}
		}
		fprintf(fp, "</table></div>\n");
	}else{
		fprintf(fp, "<div id=mainmap style=\"position:absolute; top: 0; left: 0; z-index: 1;\">"
			"<table id=map border=1 cellspacing=0 cellpadding=0 background=%s>\n", dat->mapimg);
		for(i=0; i<dat->y; i++){
			if(dat->masutype != 's' && i % 2){
				fprintf(fp, "\t<tr>\n\t\t<td class=map_td_h></td>\n");
			}else{
				fprintf(fp, "\t<tr>\n");
			}
			for(j=0; j<dat->x; j++){
				fprintf(fp, "\t\t<td class=map_td %s>(%d,%d)</td>\n", tdstr, j, i);
			}
			if(dat->masutype != 's' && !(i % 2)){
				fprintf(fp, "\t\t<td class=map_td_h></td>\n\t</tr>\n");
			}else{
				fprintf(fp, "\t</tr>\n");
			}
		}
		fprintf(fp, "</table></div>\n");
	}
	return 0;
}



int MapDataPrintEdit(MapData *dat, FILE *fp){

	int i, j;
	char *tdstr, *tdstr2;
	MapChip *chip;

	if(dat->masutype == 's'){
		tdstr = "";
	}else{
		tdstr = " colspan=2";
	}
	fprintf(fp, "<style type=text/css>\n"
		"<!--\ntd.map_td{\n"
		"width: %dpx; height: %dpx; font-size: xx-small;\n"
		"}\n"
		"td.map_td_h{\n"
		"width: %dpx; height: %dpx; font-size: xx-small;\n"
		"//-->\n</style>\n",
		dat->cw, dat->ch, dat->cw/2, dat->ch);
	if(dat->maptype == 'd'){
		chip = dat->dat;
		fprintf(fp, "<div id=mainmap style=\"position:absolute; top: 0; left: 0; z-index: 1;\">"
			"<table id=map border=1 cellspacing=0 cellpadding=0>\n");
		for(i=0; i<dat->y; i++){
			if(dat->masutype != 's' && i % 2){
				fprintf(fp, "\t<tr>\n\t\t<td class=map_td_h></td>\n");
			}else{
				fprintf(fp, "\t<tr>\n");
			}
			for(j=0; j<dat->x; j++){
				fprintf(fp, "\t\t<td class=map_td %s id=%d_%d style=\"background-image: url(%s%s);\" onmouseover=\"mapOver(%d,%d);\" onmousedown=\"mapClick(%d,%d);\">(%d,%d)</td>\n", tdstr,
					j, i, chipdir, MapChipGetFileName(GetMapChip(i,j), dat->tdat), j, i, j, i, j, i);
			}
			if(dat->masutype != 's' && !(i % 2)){
				fprintf(fp, "\t\t<td class=map_td_h></td>\n\t</tr>\n");
			}else{
				fprintf(fp, "\t</tr>\n");
			}
		}
		fprintf(fp, "</table></div>\n");
	}else{
		fprintf(fp, "<div id=mainmap style=\"position:absolute; top: 0; left: 0; z-index: 1;\">"
			"<table id=map border=1 cellspacing=0 cellpadding=0 background=%s>\n", dat->mapimg);
		for(i=0; i<dat->y; i++){
			if(dat->masutype != 's' && i % 2){
				fprintf(fp, "\t<tr>\n\t\t<td class=map_td_h></td>\n");
			}else{
				fprintf(fp, "\t<tr>\n");
			}
			for(j=0; j<dat->x; j++){
				fprintf(fp, "\t\t<td class=map_td %s>(%d,%d)</td>\n", tdstr, j, i);
			}
			if(dat->masutype != 's' && !(i % 2)){
				fprintf(fp, "\t\t<td class=map_td_h></td>\n\t</tr>\n");
			}else{
				fprintf(fp, "\t</tr>\n");
			}
		}
		fprintf(fp, "</table></div>\n");
	}
	return 0;
}
int MapDataPrintEditForm(MapData *dat, FILE *fp){
	int i;
	MapChip *chip;
	TerrainData *terr;

	fprintf(fp, "<script type=\"text/javascript\" src=\"mapedit.js\"></script>\n");
	if(dat->maptype == 'd'){
		fprintf(fp, "<script type=text/javascript><!--\n");
		fprintf(fp, "\tchipdat = new Array(");
		i = dat->x * dat->y;
		chip = dat->dat;
		while(i--){
			fprintf(fp, "%d,%d", chip->terrain, chip->num);
			if(i) fprintf(fp, ",");
			chip++;
		}
		fprintf(fp, ");\n");
		fprintf(fp, "\tterrdat = new Array(");
		i = 100;
		terr = dat->tdat;
		while(i--){
			fprintf(fp, "\'%s\'", terr->file);
			if(i) fprintf(fp, ",");
			terr++;
		}
		fprintf(fp, ");\n");
		fprintf(fp, "\tmapdat.init(%d,%d,chipdat,%d,terrdat);\n", dat->x, dat->y, 100);
		fprintf(fp, "--></script>\n");
	}
	fprintf(fp, "<form style=\"margin: 0px; float: left;\" name=mapedittool>\n");
	fprintf(fp, "<img id=selmapchip src=\"\" alt=\"\" border=1/>選択チップ<br />\n");
	fprintf(fp, "<select name=terrain onchange=\"mapTerrainChange();\" size=12>\n");
	i = 0;
	terr = dat->tdat;
	while(i < 100){
		fprintf(fp, "<option value=%d>%s\n", i, terr->name);
		terr++; i++;
	}
	fprintf(fp, "</select></form>\n");
	for(i=0; i<64; i++){
		fprintf(fp, "<img id=chip_%d src=\"\" alt=\"\" onmousedown=\"mapSelChipChange(%d);\" border=1>", i, i);
	}
	fprintf(fp, "<br style=\"clear: both;\" />\n");
	return 0;
}
/******************************************/
/* TerrainData ******************************/

TerrainData *TerrainDataCreate(FILE *fp){
	TerrainData *dat;
	char buf[256];
	int i, cnt, rv;

/*
while(fscanf(fp, "%s", buf) != EOF){
	fputs(buf, stdout);
	scanf("%s", buf);
}
//while(fgets(buf, 256, fp) != NULL) fputs(buf, stdout);
return NULL;
*/
	dat = malloc(sizeof(TerrainData) * 100);
	if(dat == NULL) return NULL;
	for(i=0; i<100; i++){
		strcpy(dat[i].name, "");
		strcpy(dat[i].file, "");
	}
	while(fgets(buf, sizeof(buf), fp)){
		KillSpace(buf);
		if(sscanf(buf, "%d\n", &cnt) != 1) continue;
		if(fgets(buf, sizeof(buf), fp) == NULL)break;
		if(cnt >= 100) continue;/* バッファ以上のナンバーは無視 */
		KillSpace(buf);
		if(strchr(buf, ',') == NULL)break;
		strncpy(dat[cnt].name, buf, (int)(strchr(buf, ',') - buf));
		KillSpace(dat[cnt].name);
		strcpy(dat[cnt].file, strchr(buf, ',') + 1);
		KillSpace(dat[cnt].file);
//fprintf(stderr, "%s, %s\n", dat[cnt].file, dat[cnt].name);
	};

	return dat;
}




/*********************************************************/
/* MapChip ***********************************************/

MapChip *MapChipSetData(MapChip *dat, int terrain, int num){
	dat->terrain = terrain;
	dat->num = num;
	return dat;
}

char *MapChipGetFileName(MapChip *dat, TerrainData *tdat){
	static char fnbuf[64];

	sprintf(fnbuf, "%s/%s%04d.%s", 
		tdat[dat->terrain].file, 
		tdat[dat->terrain].file, 
		dat->num, ext);
	return fnbuf;
}




/*********************************************************/
/* UnitData ***********************************************/


int UnitDataPrint(UnitData *dat, FILE *fp){
	if(dat->id <= 0) return 0;
	fprintf(fp, "<div id=%s%d style=\"position:absolute; top: %dpx; left: %dpx; z-index: 11;\">",
		UID, dat->id, dat->ay, dat->ax);
	fprintf(fp, "<img id=%s%d src=\"%s\" alt=\"%s\" onmousedown=\"eveUnitClick(%d)\" onmouseover=\"unitActive(%d);\" onmouseout=\"unitFree(%d);\"/>",
		UIMG, dat->id, dat->file, dat->name, dat->id, dat->id, dat->id, dat->id, dat->id);
	fprintf(fp, "<span id=%s%d style=\"background-color: #ffffff; font-weight: bold; visibility: hidden;\">%s</span>\n",
		UNAME, dat->id, dat->name);
//	fprintf(fp, "<img id=%s%d src=%s alt=\"\" width=%d height=%d />",
//		UIMG, dat->id, dat->file, dat->name, dat->cw, dat->ch);
	fprintf(fp, "<div id=%s%d style=\"background-color: #ffffff; visibility: hidden;\"><pre style=\"margin: 0px\">%s</pre></div></div>\n",
		UTXT, dat->id, dat->text);
	return 0;
}

UnitData *UnitDataCopy(UnitData *dat, UnitData *sdat){
	dat->id = sdat->id;
	dat->oid = sdat->oid;
	dat->ax = sdat->ax;
	dat->ay = sdat->ay;
	strncpy(dat->file, sdat->file, URLMAX);
	strncpy(dat->name, sdat->name, NAMEMAX);
	strncpy(dat->text, sdat->text, UNITTEXTMAX);
	return dat;
}

UnitData *UnitDataSet(UnitData *dat, int id, int oid, char *file, char *name, char *text, int ax, int ay){
	dat->id = id;
	dat->oid = oid;
	dat->ax = ax;
	dat->ay = ay;
	strncpy(dat->file, KillSpace(file), URLMAX);
	strncpy(dat->name, KillSpace(name), NAMEMAX);
	strncpy(dat->text, text, UNITTEXTMAX);
	return dat;
}



UnitData *UnitDataWrite(UnitData *dat, FILE *fp){
	if(dat->id == EOID) return NULL;
	if(dat->id != 0){
		fprintf(fp, "%d\t%d\t%d\t%d\t%s\t%s\t%s%c\n", dat->id, dat->oid, dat->ax, dat->ay, dat->file, dat->name, dat->text, 0x07);
	}
	return dat;
}
UnitData *UnitDataRead(UnitData *dat, FILE *fp){
	char *tmp, c;
	int cnt;
	if(fscanf(fp, "%d%d%d%d%s%s", &(dat->id), &(dat->oid), &(dat->ax), &(dat->ay), dat->file, dat->name) != 6){
//		ErrorPrint(stdout, "読めない");
		dat->id = EOID;
		return NULL;
	}else{
		tmp = dat->text;
		cnt = 1;
		while((c = fgetc(fp)) != EOF){
			if(UNITTEXTMAX <= cnt) break;
			if(c != 0x07){//ローカルな終端文字（まずいか？
				if(c != '\t'){
					*tmp = c;
					tmp++; cnt++;
				}
			}else{
				fgetc(fp);// 改行コードの分読む
				break;
			}
		}
		*tmp = '\0';
	}
	return dat;
}



/*********************************************************/
/* UserData ***********************************************/

void *UserDataFree(UserData *dat){
	free(dat);
}
UserData *UserDataSet(UserData *dat, int id, char type, char *name, char *password){
	dat->id = id;
	dat->type = type;
	strncpy(dat->name, KillSpace(name), NAMEMAX);
	strncpy(dat->password, KillSpace(password), PASSMAX);
	return dat;
}

UserData *UserDataRead(UserData *dat, FILE *fp){
	if(fscanf(fp, "%d\t%c\t%s\t%s\n", &(dat->id), &(dat->type), dat->name, dat->password) != 4){
		dat->id = EOID;
		return NULL;
	}
	return dat;
}
UserData *UserDataWrite(UserData *dat, FILE *fp){
	if(dat->id == EOID) return NULL;
	if(dat->id != 0){
		fprintf(fp, "%d\t%c\t%s\t%s\n", dat->id, dat->type, dat->name, dat->password);
	}
	return dat;
}


/*********************************************************/
/* SessionData ***********************************************/

void SessionDataFree(SessionData *dat){
	if(dat->mapdat) MapDataFree(dat->mapdat);
	if(dat->userdat) free(dat->unitdat);
	if(dat->unitdat) free(dat->userdat);
	free(dat);
}

SessionData *SessionDataMemInit(SessionData *dat){
	FILE *fp;
	char fstr[64];

	dat->mapdat = NULL;
	dat->userdat = NULL;
	dat->unitdat = NULL;
	dat->usermem = dat->unitmem = MALOC_SIZE;
	dat->useruse = dat->unituse = 1;

	sprintf(fstr, "%s%s", datdir, dat->mapfile);
	if((fp = flopen(fstr, "r")) != NULL){
		dat->mapdat = MapDataRead(fp, dat->maptype, dat->masutype);
		close(fp);
	}
	if(dat->mapdat == NULL){
		SessionDataFree(dat);
		return NULL;
	}
	dat->userdat = malloc(sizeof(UserData) * MALOC_SIZE);
	if(dat->userdat == NULL){
		SessionDataFree(dat);
		return NULL;
	}
	dat->unitdat = malloc(sizeof(UnitData) * MALOC_SIZE);
	if(dat->unitdat == NULL){
		SessionDataFree(dat);
		return NULL;
	}
	dat->userdat->id = EOID;
	dat->unitdat->id = EOID;
	dat->nextuserid = 1;
	dat->nextunitid = 1;
	return dat;
}

SessionData *SessionDataCreate(char *datfile, char *mapfile, char maptype, char masutype, char *title, char *username, char *password){
	SessionData *dat;
	UserData usertmp;

	dat = malloc(sizeof(SessionData));
	if(dat == NULL) return NULL;
	strncpy(dat->datfile, datfile, URLMAX);
	strncpy(dat->mapfile, mapfile, URLMAX);
	strncpy(dat->title, title, NAMEMAX);
	strncpy(dat->password, password, PASSMAX);
	dat->maptype = maptype;
	dat->masutype = masutype;
	if(SessionDataMemInit(dat) == NULL) return NULL;
	SessionDataAddUser(dat, UserDataSet(&usertmp, dat->nextuserid, 'm', username, password));
	return dat;
}

SessionData *SessionDataRead(FILE *fp){
	SessionData *dat;
	UserData usertmp;
	UnitData unittmp;
	char dumy[128], maptype[8], masutype[8];

	dat = malloc(sizeof(SessionData));
	if(dat == NULL) return NULL;
	fscanf(fp, "%s", dat->title);
	fscanf(fp, "%s%s%s%s%s", dat->datfile, dat->mapfile, dat->password, maptype, masutype);
	dat->maptype = *maptype;
	dat->masutype = *masutype;
	if(SessionDataMemInit(dat) == NULL) return NULL;

	while(UserDataRead(&usertmp, fp) != NULL) SessionDataAddUser(dat, &usertmp);
	fgets(dumy, 128,fp);
	while(UnitDataRead(&unittmp, fp) != NULL) SessionDataAddUnit(dat, &unittmp, NULL);
	return dat;
}
SessionData *SessionDataReadFile(char *datfile){
	FILE *fp;
	SessionData *dat;

	if((fp = flopen(datfile, "r"))){
		dat = SessionDataRead(fp);
		fclose(fp);
		return dat;
	}else{
		return NULL;
	}
}

int SessionDataWrite(SessionData *dat, FILE *fp){
	UnitData *unittmp;
	UserData *usertmp;

	unittmp = dat->unitdat;
	usertmp = dat->userdat;

	fprintf(fp, "%s\n", dat->title);
	fprintf(fp, "%s\t%s\t%s\t%c\t%c\n", dat->datfile, dat->mapfile, dat->password, dat->maptype, dat->masutype);
//	fprintf(fp, "#SessionDataEnd\n");
	while(UserDataWrite(usertmp, fp) != NULL) usertmp++;
	fprintf(fp, "#UserDataEnd\n");
	while(UnitDataWrite(unittmp, fp) != NULL) unittmp++;
	fprintf(fp, "#UnitDataEnd\n");
	return 0;
}


int SessionDataWriteFile(SessionData *dat){
	FILE *fp;
	char filename[64];

	sprintf(filename, "%s%s", datdir, dat->datfile);
	if(fp = flopen(filename, "w")){
		SessionDataWrite(dat, fp);
		fclose(fp);
		return 0;
	}
	return 1;
}

SessionData *SessionDataDelUser(SessionData *dat, int delid){
	UserData *tmp;

	tmp = dat->userdat;
	while(tmp->id != EOID){
		if(tmp->id == delid){
			tmp->id = 0;
			break;
		}
		tmp++;
	}
	return dat;
}

SessionData *SessionDataAddUser(SessionData *dat, UserData *adddat){
	UserData *tmp;

	if(dat->useruse >= dat->usermem - 1){
		dat->usermem += MALOC_SIZE;
		dat->userdat = realloc(dat->userdat, sizeof(UserData) * (dat->usermem));
	}
	tmp = dat->userdat;
	while(tmp->id != EOID){
		if(tmp->id == adddat->id || !strcmp(tmp->name, adddat->name)){
			if(!strcmp(tmp->password, adddat->password)){
//				UserDataSet(tmp++, adddat->id, adddat->type, adddat->name, adddat->password);
			}else{
				ErrorPrint(stdout, "パスワードが間違っています。");
			}
			return dat;
		}
		if(tmp->id >= dat->nextuserid) dat->nextuserid = tmp->id + 1;
		tmp++;
	}
	if(adddat->id <= 0) adddat->id = dat->nextuserid++;
	UserDataSet(tmp++, adddat->id, adddat->type, adddat->name, adddat->password);
	dat->useruse++;
	tmp->id = EOID;
	return dat;
}

SessionData *SessionDataDelUnit(SessionData *dat, int delid, UserData *userdat){
	UnitData *tmp;

	if(userdat == NULL) return NULL;
	tmp = dat->unitdat;
	while(tmp->id != EOID){
		if(tmp->id == delid && tmp->oid == userdat->id){
			tmp->id = 0;
			break;
		}
		tmp++;
	}
	return dat;
}

SessionData *SessionDataAddUnit(SessionData *dat, UnitData *adddat, UserData *userdat){
	UnitData *tmp;

	if(dat->unituse >= dat->unitmem - 1){
		dat->unitmem += MALOC_SIZE;
		dat->unitdat = realloc(dat->unitdat, sizeof(UnitData) * (dat->unitmem));
	}
	tmp = dat->unitdat;
	while(tmp->id != EOID){
		if(tmp->id == adddat->id || !strcmp(tmp->name, adddat->name)){
			adddat->id = tmp->id;
			if(userdat != NULL && tmp->oid == userdat->id){
				UnitDataCopy(tmp, adddat);
			}else{
				ErrorPrint(stdout, "操作できないユニットです。");
			}
			return dat;
		}
		if(tmp->id >= dat->nextunitid) dat->nextunitid = tmp->id + 1;
		tmp++;
	}
	if(adddat->id <= 0) adddat->id = dat->nextunitid++;
	UnitDataCopy(tmp++, adddat);
	dat->unituse++;
	tmp->id = EOID;
	return dat;
}


UserData *GetUserDataID(SessionData *dat, int userid, char *password){
	UserData *tmp;

	if(userid <= 0) return NULL;
	tmp = dat->userdat;
	while(tmp->id != EOID){
		if(tmp->id == userid){
			if(!strcmp(tmp->password, password)){
				return tmp;
			}else{
				return NULL;
			}
		}
		tmp++;
	}
	return NULL;
}
UserData *GetUserDataName(SessionData *dat, char *username, char *password){
	UserData *tmp;

	if(username == NULL) return NULL;
	tmp = dat->userdat;
	while(tmp->id != EOID){
		if(!strcmp(tmp->name, username)){
			if(!strcmp(tmp->password, password)){
				return tmp;
			}else{
				return NULL;
			}
		}
		tmp++;
	}
	return NULL;
}

UnitData *GetUnitDataID(SessionData *dat, int unitid){
	UnitData *tmp;

	if(unitid <= 0) return NULL;
	tmp = dat->unitdat;
	while(tmp->id != EOID){
		if(tmp->id == unitid){
			return tmp;
		}
		tmp++;
	}
	return NULL;
}
UnitData *GetUnitDataName(SessionData *dat, char *unitname){
	UnitData *tmp;

	if(unitname == NULL) return NULL;
	tmp = dat->unitdat;
	while(tmp->id != EOID){
		if(!strcmp(tmp->name, unitname)){
			return tmp;
		}
		tmp++;
	}
	return NULL;
}


/******************************************************/
/* Output **********************************************/

void HeaderPrint(FILE *fp, char *title){
	fprintf(fp, "<html>\n<head>\n"
		"<META http-equiv=\"Content-Type\" content=\"text/html; charset=Shift_JIS\">\n"
		"<META http-equiv=\"Content-Style-Type\" content=\"text/css\">\n"
		"<script type=\"text/javascript\" src=\"%s\"></script>\n", JS_NAME);
//	fprintf(fp,  "<title>%s</title>\n</head>\n<body onScroll=\"eveScroll();\">\n", title);
	fprintf(fp,  "<title>%s</title>\n</head>\n<body>\n", title);
}

void FooterPrint(FILE *fp){
	fprintf(fp, "</body>\n</html>\n");
}

void ErrorPrint(FILE *fp, char *str){
	fprintf(fp, "<p>%s</p>\n", str);
}


void PrintMainForm(FILE *fp, char *pass){
	DIR *dir;
	struct dirent  *dp;

	HeaderPrint(fp, pass);

	PrintNewSessionForm(fp, pass);

//	fprintf(fp, "%s<br>\n", getenv("REQUEST_METHOD"));
//	fprintf(fp, "%s<br>\n", getenv("QUERY_STRING"));

	fprintf(fp, "<h3>デバグ用</h3>\n");
	if(dir = opendir(datdir)){
		fprintf(fp, "<ul>\n");
		while((dp = readdir(dir))){
			fprintf(fp, "<li><a href=%s?file=%s%s>%s</a>\n", pass, datdir, dp->d_name, dp->d_name);
		}
		fprintf(fp, "</ul>\n");
		closedir(dir);
	}
	FooterPrint(fp);
}

void PrintNewSessionForm(FILE *fp, char *pass){
	fprintf(fp, "<form method=POST action=%s>\n", pass);
	fprintf(fp, "<input type=hidden name=mode value=c>");
	fprintf(fp, "<table><tr><td>HN:</td><td><input type=text name=username size=16></td><td>……管理者の名前（ＧＭ）</td></tr>\n");
	fprintf(fp, "<tr><td>MAPNAME:</td><td><input type=text name=mapname size=16></td><td>……マップ名</td></tr>\n");
	fprintf(fp, "<tr><td>PASS:</td><td><input type=text name=password size=8></td><td>……管理用のパスワード</td></tr>\n");
	fprintf(fp, "<tr><td>TYPE:</td><td><input type=radio name=masutype value=sq checked>スクエア / <input type=radio name=masutype value=hx>ヘクス</td><td>……マスの種類（ヘクスに四苦八苦</td></tr>\n");
	fprintf(fp, "<tr><td>MAPDATA:</td><td><textarea name=mapfile cols=16 rows=8></textarea></td>"
		"<td>データの種類<br><input type=radio name=maptype value=dat checked>データ(xxx.mapをテキストエディタで開いて、張る）<br>"
		"<input type=radio name=maptype value=img>画像（画像のURLを張る）"
		"xサイズ<input type=text name=mapw size=3 value=20> / "
		"yサイズ<input type=text name=maph size=3 value=20> / "
		"チップサイズ<input type=text name=mapchipsize size=3 value=32></td></tr>\n");
	fprintf(fp, "<tr><td></td><td></td><td><input type=submit value=作成></td></tr>\n");
	fprintf(fp, "</table></form>");
	fprintf(fp, "<p>長い名前とか入れると、素敵にバグるよ。誰も幸せにならんので勘弁して。10文字位なら、多分大丈夫。</p>");

	fprintf(fp, "<h3>作成済みマップ</h3>\n");
	PrintDataList(fp, pass);

}

void PrintEditForm(FILE *fp, char *pass, SessionData *dat, UserData *userdat, char *formx, char *formy){
	UnitData *unittmp;
	char *radiosel[3];

	HeaderPrint(fp, pass);

	if(userdat == NULL || userdat->type != 'm'){
		ErrorPrint(fp, "編集できません。");
		FooterPrint(fp);
		return;
	}

	if(formx == NULL) formx = "12px";
	if(formy == NULL) formy = "12px";

//	fprintf(fp, "<script type=text/javascript src=mapedit.js ></script>");
	fprintf(fp, "<div id=\"floatform\" style=\"position:absolute; background-color: #ffffff; width: 32em; top: %s; left: %s; z-index: 21;\"/>", formy, formx);
	fprintf(fp, "<div style=\"background-color: #3399ff;\" onmousedown=\"eveFormClick()\">ここをドラッグすると動く。</div>");
	fprintf(fp, "<form method=POST action=%s name=editform style=\"margin: 0px;\">\n", pass);
	fprintf(fp, "<input type=hidden name=formx value=%s>", formx);
	fprintf(fp, "<input type=hidden name=formy value=%s>", formy);
	fprintf(fp, "<input type=hidden name=mode value=r>");
	fprintf(fp, "<input type=hidden name=userid value=%d>", userdat->id);
	fprintf(fp, "<input type=hidden name=password value=%s>", userdat->password);
	fprintf(fp, "<input type=hidden name=datfile value=%s>", dat->datfile);
	fprintf(fp, "<input type=hidden name=unitid value=-1>"
		"<input type=hidden name=unitname value=>"
		"<input type=hidden name=unitfile value=>"
		"<img style=\"visibility: hidden; height: 0px;\" id=imgpreview src=\"\"/>"
		"<input type=hidden name=unittext value=>"
		"<input type=hidden name=ax value=0>"
		"<input type=hidden name=ay value=0>");
	fprintf(fp, "<table>\n");
	if(dat->mapdat->masutype == 's'){
		radiosel[0] = " checked";	radiosel[1] = "";
	}else{
		radiosel[0] = "";	radiosel[1] = " checked";
	}
	fprintf(fp, "<tr><td colspan=2>TYPE:<input type=radio name=masutype value=sq%s>スクエア / <input type=radio name=masutype value=hx%s>ヘクス</td></tr>\n",
		radiosel[0], radiosel[1]);

	fprintf(fp, "<tr><td>MAPDATA:<br /><textarea name=mapfile cols=16 rows=8>%s</textarea></td>",
		dat->mapdat->mapimg);

	if(dat->mapdat->maptype == 'd'){
		radiosel[0] = " checked";	radiosel[1] = "";
	}else{
		radiosel[0] = "";	radiosel[1] = " checked";
	}
	fprintf(fp, "<td>データの種類<br><input type=radio name=maptype onclick=\"changeMapType(\'d\')\" value=dat%s>データ(xxx.mapをテキストエディタで開いて、張る）<br>"
		"<input type=radio name=maptype onclick=\"changeMapType(\'i\')\" value=img%s>画像（画像のURLを張る）<br />",
		radiosel[0], radiosel[1]);
	fprintf(fp, "xサイズ<input type=text name=mapw size=3 value=%d> / "
		"yサイズ<input type=text name=maph size=3 value=%d> / "
		"チップサイズ<input type=text name=mapchipsize size=3 value=%d></td></tr>\n",
		dat->mapdat->x, dat->mapdat->y, dat->mapdat->cw);
	fprintf(fp, "</table><input type=submit value=更新>\n");
	fprintf(fp, "</form>");

//	if(dat->mapdat->maptype == 'd'){
		fprintf(fp, "<div class=mapedittooldiv>");
//	}else{
//		fprintf(fp, "<div class=mapedittooldiv style=\"visibility: hidden;\">");
//	}
	MapDataPrintEditForm(dat->mapdat, fp);
	fprintf(fp, "</div>");

	fprintf(fp, "<form method=POST action=%s name=setting style=\"margin: 0px;\">\n", pass);
	fprintf(fp, "<input type=hidden name=formx value=%s>", formx);
	fprintf(fp, "<input type=hidden name=formy value=%s>", formy);
	fprintf(fp, "<input type=hidden name=mode value=v>");
	fprintf(fp, "<input type=hidden name=userid value=%d>", userdat->id);
	fprintf(fp, "<input type=hidden name=password value=%s>", userdat->password);
	fprintf(fp, "<input type=hidden name=datfile value=%s>", dat->datfile);
	fprintf(fp, "<input type=hidden name=unitid value=-1>");
	fprintf(fp, "<input type=hidden name=ax value=0>"
		"<input type=hidden name=ay value=0>"
		"<input type=hidden name=unitfile value=0>"
		"<input type=hidden name=unitname value=0>"
		"<input type=hidden name=unittext value=0>");
	fprintf(fp, "<input type=submit value=編集を抜ける>\n");
//	fprintf(fp, "</form></div>");
	fprintf(fp, "</form>現状、データの種類変えちゃダメです。その内なおるかもしれませんが、現状ダメです。</div>");

	unittmp = dat->unitdat;
//	while(unittmp->id != EOID) UnitDataPrint(unittmp++, fp);
	MapDataPrintEdit(dat->mapdat, fp);

	FooterPrint(fp);
}

void PrintSessionView(FILE *fp, char *pass, SessionData *dat, UserData *userdat, char *formx, char *formy){
	UnitData *unittmp;

	HeaderPrint(fp, pass);

	if(formx == NULL) formx = "12px";
	if(formy == NULL) formy = "12px";

	fprintf(fp, "<div id=\"floatform\" style=\"position:absolute; background-color: #ffffff; width: 20em; top: %s; left: %s; z-index: 21;\"/>", formy, formx);
	fprintf(fp, "<div style=\"background-color: #3399ff;\" onmousedown=\"eveFormClick()\">ここをドラッグすると動く。</div>");
	if(userdat){
		fprintf(fp, "<form method=POST action=%s name=setting style=\"margin: 0px;\">\n", pass);
		fprintf(fp, "<input type=hidden name=formx value=%s>", formx);
		fprintf(fp, "<input type=hidden name=formy value=%s>", formy);
		fprintf(fp, "<input type=hidden name=mode value=a>");
		fprintf(fp, "<input type=hidden name=userid value=%d>", userdat->id);
		fprintf(fp, "<input type=hidden name=password value=%s>", userdat->password);
		fprintf(fp, "<input type=hidden name=datfile value=%s>", dat->datfile);
		fprintf(fp, "<input type=hidden name=unitid value=-1>");
		fprintf(fp, "UnitName:<input type=text name=unitname value=>(<input type=checkbox name=newunit value=new>新規作成)<br>");
		fprintf(fp, "UnitImage:<input type=text name=unitfile onchange=\"eveImgChange();\" value=><img id=imgpreview src=\"\" alt=プレビュー /><br>");
		fprintf(fp, "UnitText:<textarea name=unittext cols=24 rows=4></textarea><br>");
		fprintf(fp, "x:<input type=text name=ax value=0>,");
		fprintf(fp, "y:<input type=text name=ay value=0><br>");
		fprintf(fp, "<input type=submit value=更新>\n");
		fprintf(fp, "</form>");
		fprintf(fp, "自分が追加して、最後に動かしたのだけ反映される。名前欄を空欄にすると消える。");
	}else{
		fprintf(fp, "<form method=POST action=%s name=setting style=\"margin: 0px;\">\n", pass);
		fprintf(fp, "<input type=hidden name=mode value=l>");
		fprintf(fp, "HN:<input type=text name=username value=><br>");
		fprintf(fp, "PASS:<input type=text name=password value=><br>");
		fprintf(fp, "<input type=hidden name=datfile value=%s>", dat->datfile);
		fprintf(fp, "<input type=submit value=参加>\n");
		fprintf(fp, "<input type=hidden name=unitid value=-1>");
		fprintf(fp, "<input type=hidden name=ax value=0>"
			"<input type=hidden name=ay value=0>"
			"<input type=hidden name=unitfile value=0>"
			"<input type=hidden name=unitname value=0>"
			"<input type=hidden name=unittext value=0>");
		fprintf(fp, "<input type=hidden name=formx value=%s>", formx);
		fprintf(fp, "<input type=hidden name=formy value=%s>", formy);
		fprintf(fp, "</form>");
		fprintf(fp, "名前とパスワード入れてログイン");
	}
	if(userdat && userdat->type == 'm'){
		fprintf(fp, "<form method=POST action=%s name=mapedit style=\"margin: 0px;\">\n", pass);
		fprintf(fp, "<input type=hidden name=formx value=%s>", formx);
		fprintf(fp, "<input type=hidden name=formy value=%s>", formy);
		fprintf(fp, "<input type=hidden name=mode value=e>");
		fprintf(fp, "<input type=hidden name=userid value=%d>", userdat->id);
		fprintf(fp, "<input type=hidden name=password value=%s>", userdat->password);
		fprintf(fp, "<input type=hidden name=datfile value=%s>", dat->datfile);

		fprintf(fp, "<input type=submit value=マップ編集>\n");
		fprintf(fp, "</form>");
	}
	fprintf(fp, "</div>");

	unittmp = dat->unitdat;
	while(unittmp->id != EOID) UnitDataPrint(unittmp++, fp);
	MapDataPrint(dat->mapdat, fp);
//	fprintf(fp, "<div style=\"height: 200px; width: 1000px;\"></div>");
	FooterPrint(fp);
}


void PrintDataList(FILE *fp, char *pass){
	FILE *tfp;
	DIR *dir;
	struct dirent  *dp;
	char filename[64];

	if(dir = opendir(datdir)){
		fprintf(fp, "<ul>\n");
		while((dp = readdir(dir))){
			if(dp->d_name == strstr(dp->d_name, "dat")){
				sprintf(filename, "%s%s", datdir, dp->d_name);
				if((tfp = flopen(filename, "r"))){
					fscanf(tfp, "%s", filename);
					fclose(tfp);
					fprintf(fp, "<li><a href=%s?mode=v&datfile=%s>%s</a>\n", pass, dp->d_name, filename);
				}
			}
		}
		fprintf(fp, "</ul>\n");
		closedir(dir);
	}
}

void PrintMap(FILE *fp, char *mapfile){
	FILE *inp;
	MapData *dat;

	HeaderPrint(stdout, mapfile);
	if((inp = flopen(mapfile, "r")) != NULL){
		dat = MapDataRead(inp, 'd', 's');
		fclose(inp);
		if(dat == NULL){
			ErrorPrint(fp, "ファイルリードエラー");
		}else{
			MapDataPrint(dat, stdout);
			MapDataFree(dat);
		}
	}
	FooterPrint(stdout);
}

/******************************************************/
/* string */

char *KillSpace(char *str){
	char *tmp, *rp, *cmp;

//fprintf(stderr, "%s->", str);
	tmp = rp = str;
	while(*str != '\0'){
		cmp = tmp;
		if(cmp != strpbrk(tmp, " \t\r\n")){
			*str = *tmp;
			if(*str == '\0') break;
			str++; tmp++;
		}else{
			tmp = strpbrk(tmp, " \t\r\n") + 1;
		}
	}
//fprintf(stderr, "%s\n", rp);
	return rp;
}


FILE *flopen(char *file, char *mode){
	FILE *fp;

	if((fp = fopen(file, mode)) == NULL){
	HeaderPrint(stdout, "");
		ErrorPrint(stdout, "ファイルオープンエラー");
		ErrorPrint(stdout, file);
	FooterPrint(stdout);
		return NULL;
	}
	return fp;
}


