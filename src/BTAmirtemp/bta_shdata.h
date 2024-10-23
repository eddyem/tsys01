#pragma once
#ifndef __BTA_SHDATA_H__
#define __BTA_SHDATA_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>

#pragma pack(push, 4)
/*
 * Shared memory block
 */
struct SHM_Block {
    union {
        char  name[5];       // memory segment identificator
        key_t code;
    } key;
    int32_t size;             // size of memory used
    int32_t maxsize;          // size when created
    int32_t mode;             // access mode (rwxrwxrwx)
    int32_t atflag;           // connection mode (SHM_RDONLY  or 0)
    void (*init)();           // init function
    int32_t  (*check)();      // test function
    void (*close)();          // deinit function
    int32_t side;             // connection type: client/server
    int32_t id;               // connection identificator
    uint8_t *addr;            // connection address
};

extern volatile struct SHM_Block sdat;

/*
 * Command queue descriptor
 */
struct CMD_Queue {
    union {
        char  name[5];    // queue key
        key_t code;
    } key;
    int32_t mode;       // access mode (rwxrwxrwx)
    int32_t side;       // connection type (Sender/Receiver - server/client)
    int32_t id;         // connection identificator
    uint32_t acckey;    // access key (for transmission from client to server)
};

extern struct CMD_Queue mcmd;
extern struct CMD_Queue ocmd;
extern struct CMD_Queue ucmd;

void send_cmd_noarg(int);
void send_cmd_str(int, char *);
void send_cmd_i1(int, int32_t);
void send_cmd_i2(int, int32_t, int32_t);
void send_cmd_i3(int, int32_t, int32_t, int32_t);
void send_cmd_i4(int, int32_t, int32_t, int32_t, int32_t);
void send_cmd_d1(int, double);
void send_cmd_d2(int, double, double);
void send_cmd_i1d1(int, int32_t, double);
void send_cmd_i2d1(int, int32_t, int32_t, double);
void send_cmd_i3d1(int, int32_t, int32_t, int32_t, double);

/*******************************************************************************
*                             Command list                                     *
*******************************************************************************/
/*      name                             code  args          type   */
// Stop telescope
#define StopTel                           1
#define StopTeleskope()   send_cmd_noarg( 1 )
// High/low speed
#define StartHS                           2
#define StartHighSpeed()  send_cmd_noarg( 2 )
#define StartLS                           3
#define StartLowSpeed()   send_cmd_noarg( 3 )
// Timer setup (Ch7_15 or SysTimer)
#define SetTmr                            4
#define SetTimerMode(T)   send_cmd_i1   ( 4, (int)(T))
// Simulation (modeling) mode
#define SetModMod                         5
#define SetModelMode(M)   send_cmd_i1   ( 5, (int)(M))
// Azimuth speed code
#define SetCodA                           6
#define SetPKN_A(iA,sA)   send_cmd_i2   ( 6, (int)(iA),(int)(sA))
// Zenith speed code
#define SetCodZ                           7
#define SetPKN_Z(iZ)      send_cmd_i1   ( 7, (int)(iZ))
// Parangle speed code
#define SetCodP                           8
#define SetPKN_P(iP)      send_cmd_i1   ( 8, (int)(iP))
// Set Az velocity
#define SetVA                             9
#define SetSpeedA(vA)     send_cmd_d1   ( 9, (double)(vA))
// Set Z velocity
#define SetVZ                            10
#define SetSpeedZ(vZ)     send_cmd_d1   (10, (double)(vZ))
// Set P velocity
#define SetVP                            11
#define SetSpeedP(vP)     send_cmd_d1   (11, (double)(vP))
// Set new polar coordinates
#define SetAD                            12
#define SetRADec(Alp,Del) send_cmd_d2   (12, (double)(Alp),(double)(Del))
// Set new azimutal coordinates
#define SetAZ                            13
#define SetAzimZ(A,Z)     send_cmd_d2   (13, (double)(A),(double)(Z))
// Goto new object by polar coords
#define GoToAD                           14
#define GoToObject()      send_cmd_noarg(14 )
// Start steering to object by polar coords
#define MoveToAD                         15
#define MoveToObject()    send_cmd_noarg(15 )
// Go to object by azimutal coords
#define GoToAZ                           16
#define GoToAzimZ()       send_cmd_noarg(16 )
// Set A&Z for simulation
#define WriteAZ                          17
#define WriteModelAZ()    send_cmd_noarg(17 )
// Set P2 mode
#define SetModP                          18
#define SetPMode(pmod)    send_cmd_i1   (18, (int)(pmod))
// Move(+-1)/Stop(0) P2
#define P2Move                           19
#define MoveP2(dir)       send_cmd_i1   (19, (int)(dir))
// Move(+-2,+-1)/Stop(0) focus
#define FocMove                          20
#define MoveFocus(speed,time) send_cmd_i1d1(20,(int)(speed),(double)(time))
// Use/don't use pointing correction system
#define UsePCorr                         21
#define SwitchPosCorr(pc_flag) send_cmd_i1 (21, (int)(pc_flag))
// Tracking flags
#define SetTrkFlags                      22
#define SetTrkOkMode(trk_flags) send_cmd_i1 (22, (int)(trk_flags))
// Set focus (0 - primary, 1 - N1, 2 - N2)
#define SetTFoc                          23
#define SetTelFocus(N)    send_cmd_i1  ( 23, (int)(N))
// Set intrinsic move parameters by RA/Decl
#define SetVAD                           24
#define SetVelAD(VAlp,VDel) send_cmd_d2 (24, (double)(VAlp),(double)(VDel))
// Reverse Azimuth direction when pointing
#define SetRevA                          25
#define SetAzRevers(amod) send_cmd_i1   (25, (int)(amod))
// Set P2 velocity
#define SetVP2                           26
#define SetVelP2(vP2) send_cmd_d1       (26, (double)(vP2))
// Set pointing target
#define SetTarg                          27
#define SetSysTarg(Targ) send_cmd_i1    (27, (int)(Targ))
// Send message to all clients (+write into protocol)
#define SendMsg                          28
#define SendMessage(Mesg) send_cmd_str  (28, (char *)(Mesg))
// RA/Decl user correction
#define CorrAD                           29
#define DoADcorr(dAlp,dDel) send_cmd_d2 (29, (double)(dAlp),(double)(dDel))
// A/Z user correction
#define CorrAZ                           30
#define DoAZcorr(dA,dZ)    send_cmd_d2  (30, (double)(dA),(double)(dZ))
// sec A/Z user correction speed
#define SetVCAZ                          31
#define SetVCorr(vA,vZ)   send_cmd_d2   (31, (double)(vA),(double)(vZ))
// move P2 with given velocity for a given time
#define P2MoveTo                         32
#define MoveP2To(vP2,time) send_cmd_d2  (32, (double)(vP2),(double)(time))
// Go to t/Decl position
#define GoToTD                           33
#define GoToSat()        send_cmd_noarg (33 )
// Move to t/Decl
#define MoveToTD                         34
#define MoveToSat()      send_cmd_noarg (34 )
// Empty command for synchronisation
#define NullCom                          35
#define SyncCom()        send_cmd_noarg (35 )
// Button "Start"
#define StartTel                         36
#define StartTeleskope()  send_cmd_noarg(36 )
// Set telescope mode
#define SetTMod                          37
#define SetTelMode(M)     send_cmd_i1  ( 37, (int)(M))
// Turn telescope on (oil etc)
#define TelOn                            38
#define TeleskopeOn()     send_cmd_noarg(38 )
// Dome mode
#define SetModD                          39
#define SetDomeMode(dmod) send_cmd_i1   (39, (int)(dmod))
// Move(+-3,+-2,+-1)/Stop(0) dome
#define DomeMove                         40
#define MoveDome(speed,time) send_cmd_i1d1(40,(int)(speed),(double)(time))
// Set account password
#define SetPass                          41
#define SetPasswd(LPass)   send_cmd_str (41, (char *)(LPass))
// Set code of access level
#define SetLevC                          42
#define SetLevCode(Nlev,Cod) send_cmd_i2(42, (int)(Nlev),(int)(Cod))
// Set key for access level
#define SetLevK                          43
#define SetLevKey(Nlev,Key)  send_cmd_i2(43, (int)(Nlev),(int)(Key))
// Setup network
#define SetNet                           44
#define SetNetAcc(Mask,Addr) send_cmd_i2(44, (int)(Mask),(int)(Addr))
// Input meteo data
#define SetMet                           45
#define SetMeteo(m_id,m_val) send_cmd_i1d1(45,(int)(m_id),(double)(m_val))
// Cancel meteo data
#define TurnMetOff                       46
#define TurnMeteoOff(m_id)   send_cmd_i1 (46, (int)(m_id))
// Set time correction (IERS DUT1=UT1-UTC)
#define SetDUT1                          47
#define SetDtime(dT)         send_cmd_d1 (47, (double)(dT))
// Set polar motion (IERS polar motion)
#define SetPM                            48
#define SetPolMot(Xp,Yp)     send_cmd_d2 (48, (double)(Xp),(double)(Yp))
// Get SEW parameter
#define GetSEW                           49
#define GetSEWparam(Ndrv,Indx,Cnt) send_cmd_i3(49,(int)(Ndrv),(int)(Indx),(int)(Cnt))
// Set SEW parameter
#define PutSEW                           50
#define PutSEWparam(Ndrv,Indx,Key,Val) send_cmd_i4(50,(int)(Ndrv),(int)(Indx),(int)(Key),(int)(Val))
// Set lock flags
#define SetLocks                         51
#define SetLockFlags(f)      send_cmd_i1 (SetLocks, (int)(f))
// Clear lock flags
#define ClearLocks                       52
#define ClearLockFlags(f)    send_cmd_i1 (ClearLocks, (int)(f))
// Set PEP-RK bits
#define SetRKbits                        53
#define AddRKbits(f)         send_cmd_i1 (SetRKbits, (int)(f))
// Clear PEP-RK bits
#define ClrRKbits                        54
#define ClearRKbits(f)       send_cmd_i1 (ClrRKbits, (int)(f))
// Set SEW dome motor number (for indication)
#define SetSEWnd                         55
#define SetDomeDrive(ND)     send_cmd_i1 (SetSEWnd, (int)(ND))
// Turn SEW controllers of dome on/off
#define SEWsDome                         56
#define DomeSEW(OnOff)       send_cmd_i1 (SEWsDome, (int)(OnOff))


/*******************************************************************************
*                         BTA data structure definitions                       *
*******************************************************************************/

#define ServPID  (sdt->pid) // PID of main program
// model
#define UseModel  (sdt->model) // model variants
enum{
     NoModel = 0  // OFF
    ,CheckModel   // control motors by model
    ,DriveModel   // "blind" management without real sensors
    ,FullModel    // full model without telescope
};
// timer
#define ClockType (sdt->timer) // which timer to use
enum{
     Ch7_15 = 0 // Inner timer with synchronisation by CH7_15
    ,SysTimer   // System timer (synchronisation unknown)
    ,ExtSynchro // External synchronisation (bta_time or xntpd)
};
// system
#define Sys_Mode (sdt->system) // main system mode
enum{
     SysStop = 0 // Stop
    ,SysWait     // Wait for start (pointing)
    ,SysPointAZ  // Pointing by A/Z
    ,SysPointAD  // Pointing by RA/Decl
    ,SysTrkStop  // Tracking stop
    ,SysTrkStart // Start tracking (acceleration to nominal velocity)
    ,SysTrkMove  // Tracking move to object
    ,SysTrkSeek  // Tracking in seeking mode
    ,SysTrkOk    // Tracking OK
    ,SysTrkCorr  // Correction of tracking position
    ,SysTest     // Test
};
// sys_target
#define Sys_Target (sdt->sys_target) // system pointing target
enum{
     TagPosition = 0 // point by A/Z
    ,TagObject       // point by RA/Decl
    ,TagNest         // point to "nest"
    ,TagZenith       // point to zenith
    ,TagHorizon      // point to horizon
    ,TagStatObj      // point to statinary object (t/Decl)
};
// tel_focus
#define Tel_Focus (sdt->tel_focus) // telescope focus type
enum{
     Prime = 0
    ,Nasmyth1
    ,Nasmyth2
};
// PCS
#define  PosCor_Coeff  (sdt->pc_coeff) // pointing correction system coefficients
// tel_state
#define  Tel_State (sdt->tel_state) // telescope state
#define  Req_State (sdt->req_state) // required state
enum{
     Stopping = 0
    ,Pointing
    ,Tracking
};
// tel_hard_state
#define  Tel_Hardware (sdt->tel_hard_state) // Power state
enum{
     Hard_Off = 0
    ,Hard_On
};
// tel_mode
#define  Tel_Mode (sdt->tel_mode) // telescope mode
enum{
     Automatic = 0 // Automatic (normal) mode
    ,Manual    = 1 // manual mode
    ,ZenHor    = 2 // work when Z<5 || Z>80
    ,A_Move    = 4 // hand move by A
    ,Z_Move    = 8 // hand move by Z
    ,Balance  =0x10// balancing
};
// az_mode
#define Az_Mode (sdt->az_mode) // azimuth reverce
enum{
     Rev_Off = 0  // move by nearest way
    ,Rev_On       // move by longest way
};
// p2_state
#define P2_State (sdt->p2_state) // P2 motor state
#define  P2_Mode (sdt->p2_req_mode)
enum{
     P2_Off = 0    // Stop
    ,P2_On         // Guiding
    ,P2_Plus       // Move to +
    ,P2_Minus = -2 // Move to -
};
// focus_state
#define Foc_State (sdt->focus_state) // focus motor state
enum{
     Foc_Hminus = -2// fast "-" move
    ,Foc_Lminus     // slow "-" move
    ,Foc_Off        // Off
    ,Foc_Lplus      // slow "+" move
    ,Foc_Hplus      // fast "+" move
};
// dome_state
#define Dome_State (sdt->dome_state) // dome motors state
enum{
     D_Hminus = -3 // speeds: low, medium, high
    ,D_Mminus
    ,D_Lminus
    ,D_Off         // off
    ,D_Lplus
    ,D_Mplus
    ,D_Hplus
    ,D_On = 7      // auto
};
// pcor_mode
#define Pos_Corr (sdt->pcor_mode) // pointing correction mode
enum{
     PC_Off = 0
    ,PC_On
};
// trkok_mode
#define TrkOk_Mode (sdt->trkok_mode) // tracking mode
enum{
     UseDiffVel = 1 // Isodrome (correction by real motors speed)
    ,UseDiffAZ  = 2 // Tracking by coordinate difference
    ,UseDFlt    = 4 // Turn on digital filter
};
// input RA/Decl values
#define  InpAlpha  (sdt->i_alpha)
#define  InpDelta  (sdt->i_delta)
// current source RA/Decl values
#define  SrcAlpha  (sdt->s_alpha)
#define  SrcDelta  (sdt->s_delta)
// intrinsic object velocity
#define  VelAlpha  (sdt->v_alpha)
#define  VelDelta  (sdt->v_delta)
// input A/Z values
#define  InpAzim   (sdt->i_azim)
#define  InpZdist  (sdt->i_zdist)
// calculated values
#define  CurAlpha  (sdt->c_alpha)
#define  CurDelta  (sdt->c_delta)
// current values (from sensors)
#define  tag_A  (sdt->tag_a)
#define  tag_Z  (sdt->tag_z)
#define  tag_P  (sdt->tag_p)
 // calculated corrections
#define  pos_cor_A  (sdt->pcor_a)
#define  pos_cor_Z  (sdt->pcor_z)
#define  refract_Z  (sdt->refr_z)
// reverse calculation corr.
#define  tel_cor_A  (sdt->tcor_a)
#define  tel_cor_Z  (sdt->tcor_z)
#define  tel_ref_Z  (sdt->tref_z)
// coords difference
#define  Diff_A  (sdt->diff_a)
#define  Diff_Z  (sdt->diff_z)
#define  Diff_P  (sdt->diff_p)
// base object velocity
#define  vel_objA (sdt->vbasea)
#define  vel_objZ (sdt->vbasez)
#define  vel_objP (sdt->vbasep)
// correction by real speed
#define diff_vA  (sdt->diffva)
#define diff_vZ  (sdt->diffvz)
#define diff_vP  (sdt->diffvp)
// motor speed
#define  speedA  (sdt->speeda)
#define  speedZ  (sdt->speedz)
#define  speedP  (sdt->speedp)
// last precipitation time
#define  Precip_time (sdt->m_time_precip)
// reserved
#define  Reserve (sdt->reserve)
// real motor speed (''/sec)
#define  req_speedA (sdt->rspeeda)
#define  req_speedZ (sdt->rspeedz)
#define  req_speedP (sdt->rspeedp)
// model speed
#define  mod_vel_A  (sdt->simvela)
#define  mod_vel_Z  (sdt->simvelz)
#define  mod_vel_P  (sdt->simvelp)
#define  mod_vel_F  (sdt->simvelf)
#define  mod_vel_D  (sdt->simvelf)
// telescope & hand correction state
/*
 * 0x8000 - азимут положительный
 * 0x4000 - отработка вкл.
 * 0x2000 - режим ведения
 * 0x1000 - отработка P2 вкл.
 * 0x01F0 - ск.корр. 0.2 0.4 1.0 2.0 5.0("/сек)
 * 0x000F - напр.корр. +Z -Z +A -A
 */
#define  code_KOST (sdt->kost)
// different time (UTC, stellar, local)
#define  M_time  (sdt->m_time)
#define  S_time  (sdt->s_time)
#define  L_time  (sdt->l_time)
// PPNDD sensor (rough) code
#define  ppndd_A  (sdt->ppndd_a)
#define  ppndd_Z  (sdt->ppndd_z)
#define  ppndd_P  (sdt->ppndd_p)
#define  ppndd_B  (sdt->ppndd_b)  // atm. pressure
// DUP sensor (precise) code (Gray code)
#define  dup_A  (sdt->dup_a)
#define  dup_Z  (sdt->dup_z)
#define  dup_P  (sdt->dup_p)
#define  dup_F  (sdt->dup_f)
#define  dup_D  (sdt->dup_d)
// binary 14-digit precise code
#define  low_A  (sdt->low_a)
#define  low_Z  (sdt->low_z)
#define  low_P  (sdt->low_p)
#define  low_F  (sdt->low_f)
#define  low_D  (sdt->low_d)
// binary 23-digit rough code
#define  code_A  (sdt->code_a)
#define  code_Z  (sdt->code_z)
#define  code_P  (sdt->code_p)
#define  code_B  (sdt->code_b)
#define  code_F  (sdt->code_f)
#define  code_D  (sdt->code_d)
// ADC PCL818 (8-channel) codes
#define  ADC(N) (sdt->adc[(N)])
#define  code_T1 ADC(0)        // External temperature code
#define  code_T2 ADC(1)        // In-dome temperature code
#define  code_T3 ADC(2)        // Mirror temperature code
#define  code_Wnd ADC(3)       // Wind speed code
// calculated values
#define  val_A  (sdt->val_a)    // A, ''
#define  val_Z  (sdt->val_z)    // Z, ''
#define  val_P  (sdt->val_p)    // P, ''
#define  val_B  (sdt->val_b)    // atm. pressure, mm.hg.
#define  val_F  (sdt->val_f)    // focus, mm
#define  val_D  (sdt->val_d)    // Dome Az, ''
#define  val_T1 (sdt->val_t1)   // ext. T, degrC
#define  val_T2 (sdt->val_t2)   // in-dome T, degrC
#define  val_T3 (sdt->val_t3)   // mirror T, degrC
#define  val_Wnd (sdt->val_wnd) // wind speed, m/s
// RA/Decl calculated by A/Z
#define  val_Alp  (sdt->val_alp)
#define  val_Del  (sdt->val_del)
// measured speed
#define  vel_A  (sdt->vel_a)
#define  vel_Z  (sdt->vel_z)
#define  vel_P  (sdt->vel_p)
#define  vel_F  (sdt->vel_f)
#define  vel_D  (sdt->vel_d)
// system messages queue
#define MesgNum 3
#define MesgLen 39
// message type
enum{
     MesgEmpty = 0
    ,MesgInfor
    ,MesgWarn
    ,MesgFault
    ,MesgLog
};
#define Sys_Mesg(N) (sdt->sys_msg_buf[N])
// access levels
#define  code_Lev1   (sdt->code_lev[0]) // remote observer - only information
#define  code_Lev2   (sdt->code_lev[1]) // local observer - input coordinates
#define  code_Lev3   (sdt->code_lev[2]) // main observer - correction by A/Z, P2/F management
#define  code_Lev4   (sdt->code_lev[3]) // operator - start/stop telescope, testing
#define  code_Lev5   (sdt->code_lev[4]) // main operator - full access
#define  code_Lev(x) (sdt->code_lev[(x-1)])
// network settings
#define  NetMask    (sdt->netmask)  // subnet mask (usually 255.255.255.0)
#define  NetWork    (sdt->netaddr)  // subnet address (for ex.: 192.168.3.0)
#define  ACSMask    (sdt->acsmask)  // ACS network mask (for ex.: 255.255.255.0)
#define  ACSNet     (sdt->acsaddr)  // ACS subnet address (for ex.: 192.168.13.0)
// meteo data
#define  MeteoMode (sdt->meteo_stat)
enum{
     INPUT_B   = 1    // pressure
    ,INPUT_T1  = 2    // external T
    ,INPUT_T2  = 4    // in-dome T
    ,INPUT_T3  = 8    // mirror T
    ,INPUT_WND = 0x10 // wind speed
    ,INPUT_HMD = 0x20 // humidity
};
#define  SENSOR_B   (INPUT_B  <<8)  // external data flags
#define  SENSOR_T1  (INPUT_T1 <<8)
#define  SENSOR_T2  (INPUT_T2 <<8)
#define  SENSOR_T3  (INPUT_T3 <<8)
#define  SENSOR_WND (INPUT_WND<<8)
#define  SENSOR_HMD (INPUT_HMD<<8)
#define  ADC_B      (INPUT_B  <<16)  // reading from ADC flags
#define  ADC_T1     (INPUT_T1 <<16)
#define  ADC_T2     (INPUT_T2 <<16)
#define  ADC_T3     (INPUT_T3 <<16)
#define  ADC_WND    (INPUT_WND<<16)
#define  ADC_HMD    (INPUT_HMD<<16)
#define  NET_B      (INPUT_B  <<24)  // got by network flags
#define  NET_T1     (INPUT_T1 <<24)
#define  NET_T3     (INPUT_T3 <<24)
#define  NET_WND    (INPUT_WND<<24)
#define  NET_HMD    (INPUT_HMD<<24)
// input meteo values
#define  inp_B  (sdt->inp_b)    // atm.pressure (mm.hg)
#define  inp_T1 (sdt->inp_t1)   // ext T
#define  inp_T2 (sdt->inp_t2)   // in-dome T
#define  inp_T3 (sdt->inp_t3)   // mirror T
#define  inp_Wnd (sdt->inp_wnd) // wind
// values used for refraction calculation
#define  Temper      (sdt->temper)
#define  Pressure    (sdt->press)
// last wind gust time
#define  Wnd10_time  (sdt->m_time10)
#define  Wnd15_time  (sdt->m_time15)
// IERS DUT1
#define  DUT1  (sdt->dut1)
// sensors reading time
#define  A_time  (sdt->a_time)
#define  Z_time  (sdt->z_time)
#define  P_time  (sdt->p_time)
// input speeds
#define  speedAin  (sdt->speedain)
#define  speedZin  (sdt->speedzin)
#define  speedPin  (sdt->speedpin)
// acceleration (''/sec^2)
#define  acc_A  (sdt->acc_a)
#define  acc_Z  (sdt->acc_z)
#define  acc_P  (sdt->acc_p)
#define  acc_F  (sdt->acc_f)
#define  acc_D  (sdt->acc_d)
// SEW code
#define  code_SEW  (sdt->code_sew)
// sew data
#define  statusSEW(Drv) (sdt->sewdrv[(Drv)-1].status)
#define  statusSEW1     (sdt->sewdrv[0].status)
#define  statusSEW2     (sdt->sewdrv[1].status)
#define  statusSEW3     (sdt->sewdrv[2].status)
#define  speedSEW(Drv) (sdt->sewdrv[(Drv)-1].set_speed)
#define  speedSEW1     (sdt->sewdrv[0].set_speed)
#define  speedSEW2     (sdt->sewdrv[1].set_speed)
#define  speedSEW3     (sdt->sewdrv[2].set_speed)
#define  vel_SEW(Drv) (sdt->sewdrv[(Drv)-1].mes_speed)
#define  vel_SEW1     (sdt->sewdrv[0].mes_speed)
#define  vel_SEW2     (sdt->sewdrv[1].mes_speed)
#define  vel_SEW3     (sdt->sewdrv[2].mes_speed)
#define  currentSEW(Drv) (sdt->sewdrv[(Drv)-1].current)
#define  currentSEW1     (sdt->sewdrv[0].current)
#define  currentSEW2     (sdt->sewdrv[1].current)
#define  currentSEW3     (sdt->sewdrv[2].current)
#define  indexSEW(Drv) (sdt->sewdrv[(Drv)-1].index)
#define  indexSEW1     (sdt->sewdrv[0].index)
#define  indexSEW2     (sdt->sewdrv[1].index)
#define  indexSEW3     (sdt->sewdrv[2].index)
#define  valueSEW(Drv) (sdt->sewdrv[(Drv)-1].value.l)
#define  valueSEW1     (sdt->sewdrv[0].value.l)
#define  valueSEW2     (sdt->sewdrv[1].value.l)
#define  valueSEW3     (sdt->sewdrv[2].value.l)
#define  bvalSEW(Drv,Nb) (sdt->sewdrv[(Drv)-1].value.b[Nb])
// 23-digit PEP-controllers code
#define  PEP_code_A  (sdt->pep_code_a)
#define  PEP_code_Z  (sdt->pep_code_z)
#define  PEP_code_P  (sdt->pep_code_p)
// PEP end-switches code
#define  switch_A  (sdt->pep_sw_a)
enum{
     Sw_minus_A    = 1  // negative A value
    ,Sw_plus240_A  = 2  // end switch +240degr
    ,Sw_minus240_A = 4  // end switch -240degr
    ,Sw_minus45_A  = 8  // "horizon" end switch
};
#define  switch_Z  (sdt->pep_sw_z)
enum{
     Sw_0_Z    = 1
    ,Sw_5_Z    = 2
    ,Sw_20_Z   = 4
    ,Sw_60_Z   = 8
    ,Sw_80_Z   = 0x10
    ,Sw_90_Z   = 0x20
};
#define  switch_P  (sdt->pep_sw_p)
enum{
     Sw_No_P = 0    // no switches
    ,Sw_22_P = 1    // 22degr
    ,Sw_89_P = 2    // 89degr
    ,Sw_Sm_P = 0x80 // Primary focus smoke sensor
};
// PEP codes
#define  PEP_code_F    (sdt->pep_code_f)
#define  PEP_code_D    (sdt->pep_code_d)
#define  PEP_code_Rin  (sdt->pep_code_ri)
#define  PEP_code_Rout (sdt->pep_code_ro)
// PEP flags
#define PEP_A_On  (sdt->pep_on[0])
#define PEP_A_Off (PEP_A_On==0)
#define PEP_Z_On  (sdt->pep_on[1])
#define PEP_Z_Off (PEP_Z_On==0)
#define PEP_P_On  (sdt->pep_on[2])
#define PEP_P_Off (PEP_P_On==0)
#define PEP_F_On  (sdt->pep_on[3])
#define PEP_F_Off (PEP_F_On==0)
#define PEP_D_On  (sdt->pep_on[4])
#define PEP_D_Off (PEP_D_On==0)
#define PEP_R_On  (sdt->pep_on[5])
#define PEP_R_Off ((PEP_R_On&1)==0)
#define PEP_R_Inp ((PEP_R_On&2)!=0)
#define PEP_K_On  (sdt->pep_on[6])
#define PEP_K_Off ((PEP_K_On&1)==0)
#define PEP_K_Inp ((PEP_K_On&2)!=0)
// IERS polar motion
#define  polarX (sdt->xpol)
#define  polarY (sdt->ypol)
// current Julian date, sidereal time correction by "Equation of the Equinoxes"
#define JDate   (sdt->jdate)
#define EE_time (sdt->eetime)
// humidity value (%%) & hand input
#define  val_Hmd (sdt->val_hmd)
#define  inp_Hmd (sdt->val_hmd)
// worm position, mkm
#define  worm_A (sdt->worm_a)
#define  worm_Z (sdt->worm_z)
// locking flags
#define LockFlags  (sdt->lock_flags)
enum{
     Lock_A = 1
    ,Lock_Z = 2
    ,Lock_P = 4
    ,Lock_F = 8
    ,Lock_D = 0x10
};
#define A_Locked   (LockFlags&Lock_A)
#define Z_Locked   (LockFlags&Lock_Z)
#define P_Locked   (LockFlags&Lock_P)
#define F_Locked   (LockFlags&Lock_F)
#define D_Locked   (LockFlags&Lock_D)
// SEW dome divers speed
#define Dome_Speed (sdt->sew_dome_speed)
// SEW dome drive number (for indication)
#define DomeSEW_N  (sdt->sew_dome_num)
// SEW dome driver parameters
#define  statusSEWD  (sdt->sewdomedrv.status)    // controller status
#define  speedSEWD   (sdt->sewdomedrv.set_speed) // speed, rpm
#define  vel_SEWD    (sdt->sewdomedrv.mes_speed) /*измеренная скорость об/мин (rpm)*/
#define  currentSEWD (sdt->sewdomedrv.current)   // current, A
#define  indexSEWD   (sdt->sewdomedrv.index)     // parameter index
#define  valueSEWD   (sdt->sewdomedrv.value.l)   // parameter value
// dome PEP codes
#define PEP_code_Din  (sdt->pep_code_di) // data in
#define PEP_Dome_SEW_Ok   0x200
#define PEP_Dome_Cable_Ok 0x100
#define PEP_code_Dout (sdt->pep_code_do) // data out
#define PEP_Dome_SEW_On   0x10
#define PEP_Dome_SEW_Off  0x20


/*******************************************************************************
*                         BTA data structure                                   *
*******************************************************************************/

#define BTA_Data_Ver 2
struct BTA_Data {
    int32_t magic;                 // magic value
    int32_t version;               // BTA_Data_Ver
    int32_t size;                  // sizeof(struct BTA_Data)
    int32_t pid;                   // main process PID
    int32_t model;                 // model modes
    int32_t timer;                 // timer selected
    int32_t system;                // main system mode
    int32_t sys_target;            // system pointing target
    int32_t tel_focus;             // telescope focus type
    double pc_coeff[8];            // pointing correction system coefficients
    int32_t tel_state;             // telescope state
    int32_t req_state;             // new (required) state
    int32_t tel_hard_state;        // Power state
    int32_t tel_mode;              // telescope mode
    int32_t az_mode;               // azimuth reverce
    int32_t p2_state;              // P2 motor state
    int32_t p2_req_mode;           // P2 required state
    int32_t focus_state;           // focus motor state
    int32_t dome_state;            // dome motors state
    int32_t pcor_mode;             // pointing correction mode
    int32_t trkok_mode;            // tracking mode
    double i_alpha, i_delta;       // input values
    double s_alpha, s_delta;       // source
    double v_alpha, v_delta;       // intrinsic vel.
    double i_azim, i_zdist;        // input A/Z
    double c_alpha, c_delta;       // calculated values
    double tag_a, tag_z, tag_p;    // current values (from sensors)
    double pcor_a, pcor_z, refr_z; // calculated corrections
    double tcor_a, tcor_z, tref_z; // reverse calculation corr.
    double diff_a, diff_z, diff_p; // coords difference
    double vbasea,vbasez,vbasep;   // base object velocity
    double diffva,diffvz,diffvp;   // correction by real speed
    double speeda,speedz,speedp;   // motor speed
    double m_time_precip;          // last precipitation time
    uint8_t reserve[16];           // reserved
    double rspeeda, rspeedz, rspeedp; // real motor speed (''/sec)
    double simvela, simvelz, simvelp, simvelf, simveld; // model speed
    uint32_t kost;                 // telescope & hand correction state
    double m_time, s_time, l_time; // different time (UTC, stellar, local)
    uint32_t ppndd_a, ppndd_z, ppndd_p, ppndd_b; // PPNDD sensor (rough) code
    uint32_t dup_a, dup_z, dup_p, dup_f, dup_d;  // DUP sensor (precise) code (Gray code)
    uint32_t low_a, low_z, low_p, low_f, low_d;  // binary 14-digit precise code
    uint32_t code_a, code_z, code_p, code_b, code_f, code_d; // binary 23-digit rough code
    uint32_t adc[8];               // ADC PCL818 (8-channel) codes
    double val_a, val_z, val_p, val_b, val_f, val_d;
    double val_t1, val_t2, val_t3, val_wnd; // calculated values
    double val_alp, val_del;       // RA/Decl calculated by A/Z
    double vel_a, vel_z, vel_p, vel_f, vel_d; // measured speed
    // system messages queue
    struct SysMesg {
        int32_t seq_num;
        char type;                  // message type
        char text[MesgLen];         // message itself
    } sys_msg_buf[MesgNum];
    // access levels
    uint32_t code_lev[5];
    // network settings
    uint32_t netmask, netaddr, acsmask, acsaddr;
    int32_t meteo_stat;            // meteo data
    double inp_b, inp_t1, inp_t2, inp_t3, inp_wnd; // input meteo values
    double temper, press;          // values used for refraction calculation
    double m_time10, m_time15;     // last wind gust time
    double dut1; // IERS DUT1  (src: ftp://maia.usno.navy.mil/ser7/ser7.dat), DUT1 = UT1-UTC
    double a_time, z_time, p_time; // sensors reading time
    double speedain, speedzin, speedpin; // input speeds
    double acc_a, acc_z, acc_p, acc_f, acc_d; // acceleration (''/sec^2)
    uint32_t code_sew;             // SEW code
    struct SEWdata {               // sew data
        int32_t status;
        double set_speed;          // target speed, rpm
        double mes_speed;          // measured speed, rpm
        double current;            // measured current, A
        int32_t index;             // parameter number
        union{                     // parameter code
            uint8_t b[4];
            uint32_t l;
        } value;
    } sewdrv[3];
    uint32_t pep_code_a, pep_code_z, pep_code_p; // 23-digit PEP-controllers code
    uint32_t pep_sw_a, pep_sw_z, pep_sw_p; // PEP end-switches code
    uint32_t pep_code_f, pep_code_d, pep_code_ri, pep_code_ro; // PEP codes
    uint8_t pep_on[10];                // PEP flags
    double xpol, ypol;                 // IERS polar motion (src: ftp://maia.usno.navy.mil/ser7/ser7.dat)
    double  jdate, eetime;             // current Julian date, sidereal time correction by "Equation of the Equinoxes"
    double val_hmd, inp_hmd;           // humidity value (%%) & hand input
    double worm_a, worm_z;             // worm position, mkm
    /* флаги блокировки управления узлами */
    uint32_t lock_flags;               // locking flags
    int32_t sew_dome_speed;            // SEW dome divers speed: D_Lplus, D_Hminus etc
    int32_t sew_dome_num;              // SEW dome drive number (for indication)
    struct SEWdata  sewdomedrv;        // SEW dome driver parameters
    uint32_t pep_code_di, pep_code_do; // dome PEP codes
};

extern volatile struct BTA_Data *sdt;

/*******************************************************************************
*                       Local data structure                                   *
*******************************************************************************/
// Oil pressure, MPa
#define PressOilA    (sdtl->pr_oil_a)
#define PressOilZ    (sdtl->pr_oil_z)
#define PressOilTank (sdtl->pr_oil_t)
// Oil themperature, degrC
#define OilTemper1   (sdtl->t_oil_1)  // oil
#define OilTemper2   (sdtl->t_oil_2)  // water

// Local data structure
struct BTA_Local {
    uint8_t reserve[120];        // reserved data
    double pr_oil_a,pr_oil_z,pr_oil_t; // Oil pressure
    double t_oil_1,t_oil_2;            // Oil themperature
};

/**
 * Message buffer structure
 */
struct my_msgbuf {
    int32_t mtype;    // message type
    uint32_t acckey;  // client access key
    uint32_t src_pid; // source PID
    uint32_t src_ip;  // IP of command source or 0 for local
    char mtext[100];  // message itself
};

extern volatile struct BTA_Local *sdtl;
extern int snd_id;
extern int cmd_src_pid;
extern uint32_t cmd_src_ip;

#define ClientSide 0
#define ServerSide 1

#ifndef BTA_MODULE
void bta_data_init();
int  bta_data_check();
void bta_data_close();
int get_shm_block(volatile struct SHM_Block *sb, int server);
int close_shm_block(volatile struct SHM_Block *sb);
void get_cmd_queue(struct CMD_Queue *cq, int server);
#endif

int check_shm_block(volatile struct SHM_Block *sb);

void encode_lev_passwd(char *passwd, int nlev, uint32_t *keylev, uint32_t *codlev);
int find_lev_passwd(char *passwd, uint32_t *keylev, uint32_t *codlev);
int check_lev_passwd(char *passwd);
void set_acckey(uint32_t newkey);

// restore packing
#pragma pack(pop)
//#pragma GCC diagnostic pop

#endif // __BTA_SHDATA_H__
