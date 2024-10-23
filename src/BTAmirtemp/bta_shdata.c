#include "bta_shdata.h"
#include "usefull_macros.h"

#include <crypt.h>

#pragma pack(push, 4)
// Main command channel (level 5)
struct CMD_Queue mcmd = {{"Mcmd"}, 0200,0,-1,0};
// Operator command channel (level 4)
struct CMD_Queue ocmd = {{"Ocmd"}, 0200,0,-1,0};
// User command channel (level 2/3)
struct CMD_Queue ucmd = {{"Ucmd"}, 0200,0,-1,0};

#define MSGLEN  (80)
static char msg[MSGLEN];
#define PERR(...)  do{snprintf(msg, MSGLEN, __VA_ARGS__); perror(msg);} while(0)

#ifndef BTA_MODULE
volatile struct BTA_Data *sdt;
volatile struct BTA_Local *sdtl;

volatile struct SHM_Block sdat = {
	{"Sdat"},
	sizeof(struct BTA_Data),
	2048,0444,
	SHM_RDONLY,
	bta_data_init,
	bta_data_check,
	bta_data_close,
	ClientSide,-1,NULL
};

int snd_id = -1;        // client sender ID
int cmd_src_pid = 0;    // next command source PID
uint32_t cmd_src_ip = 0;// next command source IP

/**
 * Init data
 */
void bta_data_init() {
	sdt = (struct BTA_Data *)sdat.addr;
	sdtl = (struct BTA_Local *)(sdat.addr+sizeof(struct BTA_Data));
	if(sdat.side == ClientSide) {
		if(sdt->magic != sdat.key.code) {
			WARN("Wrong shared data (maybe server turned off)");
		}
		if(sdt->version == 0) {
			WARN("Null shared data version (maybe server turned off)");
		}
		else if(sdt->version != BTA_Data_Ver) {
			WARN("Wrong shared data version: I'am - %d, but server - %d ...",
				BTA_Data_Ver, sdt->version );
		}
		if(sdt->size != sdat.size) {
			if(sdt->size > sdat.size) {
				WARN("Wrong shared area size: I needs - %d, but server - %d ...",
					sdat.size, sdt->size );
			} else {
				WARN("Attention! Too little shared data structure!");
				WARN("I needs - %d, but server gives only %d ...",
					sdat.size, sdt->size );
				WARN("May be server's version too old!?");
			}
		}
		return;
	}
	/* ServerSide */
	if(sdt->magic == sdat.key.code  &&
		sdt->version == BTA_Data_Ver &&
		sdt->size == sdat.size)
		return;
	memset(sdat.addr, 0, sdat.maxsize);
	sdt->magic = sdat.key.code;
	sdt->version = BTA_Data_Ver;
	sdt->size = sdat.size;
	Tel_Hardware = Hard_On;
	Pos_Corr = PC_On;
	TrkOk_Mode = UseDiffVel | UseDiffAZ ;
	inp_B = 591.;
	Pressure  = 595.;
	PEP_code_A = 0x002aaa;
	PEP_code_Z = 0x002aaa;
	PEP_code_P = 0x002aaa;
	PEP_code_F = 0x002aaa;
	PEP_code_D = 0x002aaa;
	DomeSEW_N = 1;
}

int  bta_data_check() {
	return( (sdt->magic == sdat.key.code) && (sdt->version == BTA_Data_Ver) );
}

void bta_data_close() {
	if(sdat.side == ServerSide) {
		sdt->magic = 0;
		sdt->version = 0;
	}
}

/**
 * Allocate shared memory segment
 */
int get_shm_block(volatile struct SHM_Block *sb, int server) {
	int getsize = (server)? sb->maxsize : sb->size;
	// first try to find existing one
	sb->id = shmget(sb->key.code, getsize, sb->mode);
	if(sb->id < 0 && errno == ENOENT && server){
		// if no - try to create a new one
		int cresize = sb->maxsize;
		if(sb->size > cresize){
			WARN("Wrong shm maxsize(%d) < realsize(%d)",sb->maxsize,sb->size);
			cresize = sb->size;
		}
		sb->id = shmget(sb->key.code, cresize, IPC_CREAT|IPC_EXCL|sb->mode);
	}
	if(sb->id < 0){
		if(server)
			PERR("Can't create shared memory segment '%s'",sb->key.name);
		else
			PERR("Can't find shared segment '%s' (maybe no server process) ",sb->key.name);
		return 0;
	}
	// attach it to our memory space
	sb->addr = (unsigned char *) shmat(sb->id, NULL, sb->atflag);
	if((long)sb->addr == -1){
		PERR("Can't attach shared memory segment '%s'",sb->key.name);
		return 0;
	}
	if(server && (shmctl(sb->id, SHM_LOCK, NULL) < 0)){
		PERR("Can't prevents swapping of shared memory segment '%s'",sb->key.name);
		return 0;
	}
	DBG("Create & attach shared memory segment '%s' %dbytes", sb->key.name, sb->size);
	sb->side = server;
	if(sb->init != NULL)
		sb->init();
	return 1;
}

int close_shm_block(volatile struct SHM_Block *sb){
	int ret;
	if(sb->close != NULL)
		sb->close();
	if(sb->side == ServerSide) {
	//      ret = shmctl(sb->id, SHM_UNLOCK, NULL);
		ret = shmctl(sb->id, IPC_RMID, NULL);
	}
	ret = shmdt (sb->addr);
	return(ret);
}

/**
 * Create|Find command queue
 */
void get_cmd_queue(struct CMD_Queue *cq, int server){
	if (!server && cq->id >= 0) { //if already in use set current
		snd_id = cq->id;
		return;
	}
	// first try to find existing one
	cq->id = msgget(cq->key.code, cq->mode);
	// if no - try to create a new one
	if(cq->id<0 && errno == ENOENT && server)
		cq->id = msgget(cq->key.code, IPC_CREAT|IPC_EXCL|cq->mode);
	if(cq->id<0){
		if(server)
			PERR("Can't create comand queue '%s'",cq->key.name);
		else
			PERR("Can't find comand queue '%s' (maybe no server process) ",cq->key.name);
		return;
	}
	cq->side = server;
	if(server){
		char buf[120];  /* выбросить все команды из очереди */
		while(msgrcv(cq->id, (struct msgbuf *)buf, 112, 0, IPC_NOWAIT) > 0);
	}else
		snd_id = cq->id;
	cq->acckey = 0;
}

#endif // BTA_MODULE


int check_shm_block(volatile struct SHM_Block *sb) {
	if(sb->check)
		return(sb->check());
	else return(0);
}

/**
 * Set access key in current channel
 */
void set_acckey(uint32_t newkey){
	if(snd_id < 0) return;
	if(ucmd.id == snd_id)      ucmd.acckey = newkey;
	else if(ocmd.id == snd_id) ocmd.acckey = newkey;
	else if(mcmd.id == snd_id) mcmd.acckey = newkey;
}

/**
 * Setup source data for one following command if default values
 * (IP == 0 - local, PID = current) not suits
 */
void set_cmd_src(uint32_t ip, int pid) {
	cmd_src_pid = pid;
	cmd_src_ip = ip;
}

#pragma pack(push, 4)
/**
 * Send client commands to server
 */
void send_cmd(int cmd_code, char *buf, int size) {
	struct my_msgbuf mbuf;
	if(snd_id < 0) return;
	if(size > 100) size = 100;
	if(cmd_code > 0)
		mbuf.mtype = cmd_code;
	else
		return;
	if(ucmd.id == snd_id)      mbuf.acckey = ucmd.acckey;
	else if(ocmd.id == snd_id) mbuf.acckey = ocmd.acckey;
	else if(mcmd.id == snd_id) mbuf.acckey = mcmd.acckey;

	mbuf.src_pid = cmd_src_pid ? cmd_src_pid : getpid();
	mbuf.src_ip = cmd_src_ip;
	cmd_src_pid = cmd_src_ip = 0;

	if(size > 0)
		memcpy(mbuf.mtext, buf, size);
	else {
		mbuf.mtext[0] = 0;
		size = 1;
	}
	msgsnd(snd_id, (struct msgbuf *)&mbuf, size+12, IPC_NOWAIT);
}

void send_cmd_noarg(int cmd_code) {
	send_cmd(cmd_code, NULL, 0);
}
void send_cmd_str(int cmd_code, char *arg) {
	send_cmd(cmd_code, arg, strlen(arg)+1);
}
void send_cmd_i1(int cmd_code, int32_t arg1) {
	send_cmd(cmd_code, (char *)&arg1, sizeof(int32_t));
}
void send_cmd_i2(int cmd_code, int32_t arg1, int32_t arg2) {
	int32_t ibuf[2];
	ibuf[0] = arg1;
	ibuf[1] = arg2;
	send_cmd(cmd_code, (char *)ibuf, 2*sizeof(int32_t));
}
void send_cmd_i3(int cmd_code, int32_t arg1, int32_t arg2, int32_t arg3) {
	int32_t ibuf[3];
	ibuf[0] = arg1;
	ibuf[1] = arg2;
	ibuf[2] = arg3;
	send_cmd(cmd_code, (char *)ibuf, 3*sizeof(int32_t));
}
void send_cmd_i4(int cmd_code, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4) {
	int32_t ibuf[4];
	ibuf[0] = arg1;
	ibuf[1] = arg2;
	ibuf[2] = arg3;
	ibuf[3] = arg4;
	send_cmd(cmd_code, (char *)ibuf, 4*sizeof(int32_t));
}
void send_cmd_d1(int32_t cmd_code, double arg1) {
	send_cmd(cmd_code, (char *)&arg1, sizeof(double));
}
void send_cmd_d2(int cmd_code, double arg1, double arg2) {
	double dbuf[2];
	dbuf[0] = arg1;
	dbuf[1] = arg2;
	send_cmd(cmd_code, (char *)dbuf, 2*sizeof(double));
}
void send_cmd_i1d1(int cmd_code, int32_t arg1, double arg2) {
	struct {
		int32_t ival;
		double dval;
	} buf;
	buf.ival = arg1;
	buf.dval = arg2;
	send_cmd(cmd_code, (char *)&buf, sizeof(buf));
}
void send_cmd_i2d1(int cmd_code, int32_t arg1, int32_t arg2, double arg3) {
	struct {
		int32_t ival[2];
		double dval;
	} buf;
	buf.ival[0] = arg1;
	buf.ival[1] = arg2;
	buf.dval = arg3;
	send_cmd(cmd_code, (char *)&buf, sizeof(buf));
}
void send_cmd_i3d1(int cmd_code, int32_t arg1, int32_t arg2, int32_t arg3, double arg4) {
	struct {
		int32_t ival[3];
		double dval;
	} buf;
	buf.ival[0] = arg1;
	buf.ival[1] = arg2;
	buf.ival[2] = arg3;
	buf.dval = arg4;
	send_cmd(cmd_code, (char *)&buf, sizeof(buf));
}

void encode_lev_passwd(char *passwd, int nlev, uint32_t *keylev, uint32_t *codlev){
	char salt[4];
	char *encr;
	union {
		uint32_t ui;
		char c[4];
	} key, cod;
	sprintf(salt,"L%1d",nlev);
	encr = (char *)crypt(passwd, salt);
	cod.c[0] = encr[2];
	key.c[0] = encr[3];
	cod.c[1] = encr[4];
	key.c[1] = encr[5];
	cod.c[2] = encr[6];
	key.c[2] = encr[7];
	cod.c[3] = encr[8];
	key.c[3] = encr[9];
	*keylev = key.ui;
	*codlev = cod.ui;
}

int find_lev_passwd(char *passwd, uint32_t *keylev, uint32_t *codlev){
	int nlev;
	for(nlev = 5; nlev > 0; --nlev){
		encode_lev_passwd(passwd, nlev, keylev, codlev);
		if(*codlev == code_Lev(nlev)) break;
	}
	return(nlev);
}

int check_lev_passwd(char *passwd){
	uint32_t keylev,codlev;
	int nlev;
	nlev = find_lev_passwd(passwd, &keylev, &codlev);
	if(nlev > 0) set_acckey(keylev);
	return(nlev);
}

#pragma pack(pop)
