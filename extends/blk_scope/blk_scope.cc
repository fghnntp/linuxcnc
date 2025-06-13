#include "rtapi.h"
#include "hal.h"
#include "emcpos.h"
#include "emcmotcfg.h"
#include "motion.h"
#include "blk_scope.hh"
#include <iostream>
#include <vector>
#include <math.h>
#include "gather_sender.hh"
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include "AxisStrategyFatory.h"

static int blk_scope_id;
static GatherSender *s_gatherSender = nullptr;

struct input_data {
    hal_float_t *cmd_pos_x;
    hal_float_t *cmd_pos_y;
    hal_float_t *cmd_pos_z;
    hal_float_t *cmd_pos_a;
    hal_float_t *cmd_pos_b;
    hal_float_t *cmd_pos_c;
    hal_float_t *fb_pos_x;
    hal_float_t *fb_pos_y;
    hal_float_t *fb_pos_z;
    hal_float_t *fb_pos_a;
    hal_float_t *fb_pos_b;
    hal_float_t *fb_pos_c;
    hal_float_t *cmd_pos_tcp;
    hal_float_t *fb_pos_tcp;
    //control area
    hal_s32_t *ctrl_cmd;
};

input_data *input_data_ins;

enum AXIS_NAME {
    kX,
    kY,
    kZ,
    kA,
    kB,
    kC,
    kAxisNum,
};

static double s_cmdPos[kAxisNum];
static double s_actPos[kAxisNum];
static double s_tcpPos[2];
static IServoStrategy* s_servoStragegy[kAxisNum];

enum BLK_SCOPE_CTRL_STS {
    kStopSts,
    kStartSts,
    kClearRes,
    kStsNum,
};

//This will return the KAxisNum*2 , which is RTCP
static void GetCmdPos(struct emcmot_status_t *status,
    struct emcmot_config_t *config,
    double cmd_pos[]) {
    int joint_num;
    double positions[EMCMOT_MAX_JOINTS];
    emcmot_joint_t *joints = GetEMCMotJoints();

    /* copy joint position feedback to local array */
    for (joint_num = 0; joint_num < config->numJoints; joint_num++) {
	/* point to joint struct */
	emcmot_joint_t *joint = &joints[joint_num];
	/* copy coarse command */
	positions[joint_num] = joint->pos_cmd;
    }

       /* if less than a full complement of joints, zero out the rest */
    while ( joint_num < EMCMOT_MAX_JOINTS ) {
        positions[joint_num++] = 0.0;
    }

    for (int axis = 0; axis < kAxisNum; axis++) {
        cmd_pos[axis] = positions[axis];
    }

    EmcPose carte_pos_cmd;
    ZERO_EMC_POSE(carte_pos_cmd);
    BlkForward(positions, &carte_pos_cmd);
    
    cmd_pos[kAxisNum + kX] = carte_pos_cmd.tran.x;
    cmd_pos[kAxisNum + kY] = carte_pos_cmd.tran.y;
    cmd_pos[kAxisNum + kZ] = carte_pos_cmd.tran.z;
    cmd_pos[kAxisNum + kA] = carte_pos_cmd.a;
    cmd_pos[kAxisNum + kB] = carte_pos_cmd.b;
    cmd_pos[kAxisNum + kC] = carte_pos_cmd.c;

    // EmcPose *carte_pos_cmd_p = &status->carte_pos_cmd;

    // cmd_pos[kAxisNum + kX] = carte_pos_cmd_p->tran.x;
    // cmd_pos[kAxisNum + kY] = carte_pos_cmd_p->tran.y;
    // cmd_pos[kAxisNum + kZ] = carte_pos_cmd_p->tran.z;
    // cmd_pos[kAxisNum + kA] = carte_pos_cmd_p->a;
    // cmd_pos[kAxisNum + kB] = carte_pos_cmd_p->b;
    // cmd_pos[kAxisNum + kC] = carte_pos_cmd_p->c;
}

//Request All Servo Moduel Run
static void ServoProcess(double *cmd_pos, double *act_pos)
{
    for (int axis = kX; axis < kAxisNum; axis++) {
        s_servoStragegy[axis]->SetCmd(cmd_pos[axis]);
        s_servoStragegy[axis]->Step();
        act_pos[axis] = s_servoStragegy[axis]->GetAct();
    }
}

static void GetActPos(double act_pos[])
{
    int joint_num;
    double positions[EMCMOT_MAX_JOINTS];
    
    for (int joint_num= 0; joint_num < kAxisNum; joint_num++) {
        //copy joint xyzabc position
        positions[joint_num] = act_pos[joint_num];
    }

    /* if less than a full complement of joints, zero out the rest */
    while ( joint_num < EMCMOT_MAX_JOINTS ) {
        positions[joint_num++] = 0.0;
    }

    double rtcp_pos = 0.0;
    EmcPose carte_pos_cmd;
    ZERO_EMC_POSE(carte_pos_cmd);
    BlkForward(positions, &carte_pos_cmd);
    
    act_pos[kAxisNum + kX] = carte_pos_cmd.tran.x;
    act_pos[kAxisNum + kY] = carte_pos_cmd.tran.y;
    act_pos[kAxisNum + kZ] = carte_pos_cmd.tran.z;
    act_pos[kAxisNum + kA] = carte_pos_cmd.a;
    act_pos[kAxisNum + kB] = carte_pos_cmd.b;
    act_pos[kAxisNum + kC] = carte_pos_cmd.c;
}

static void GetResStr(double cmd_pos[], double act_pos[], std::string& str)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6); // 保证6位小数且非科学计数法

    // 拼接第一个数组
    for (size_t axis = kX; axis < kAxisNum; ++axis) {
        if (axis != kX) oss << " "; // 用空格分隔
        switch (axis) {
        case kX:
            oss << "X ";
            break;
        case kY:
            oss << "Y ";
            break;
        case kZ:
            oss << "Z ";
            break;
        case kA:
            oss << "A ";
            break;
        case kB:
            oss << "B ";
            break;
        case kC:
            oss << "C ";
            break;
        default:
            break;
        }
        oss << cmd_pos[axis] << " ";
        oss << act_pos[axis] << " ";
        oss << cmd_pos[kAxisNum + axis] << " ";
        oss << act_pos[kAxisNum + axis] << " ";

    }

    str = oss.str();
}

void catch_cycle(void *arg, long period)
{
    double positions[EMCMOT_MAX_JOINTS];
    EmcPose *carte_pos_cmd;	/* commanded Cartesian position */
    EmcPose *carte_pos_fb;	/* actual Cartesian position */
    struct emcmot_status_t *emcmotStatus;
    struct emcmot_config_t *emcmotConfig;
    carte_pos_cmd = &emcmotStatus->carte_pos_cmd;
    carte_pos_fb = &emcmotStatus->carte_pos_fb;
    emcmotStatus = GetEMCMotStatus();
    emcmotConfig = GetEMCMotConfig();
    double cmd_pos[kAxisNum + kAxisNum];
    double act_pos[kAxisNum + kAxisNum];
    int cmd = *input_data_ins->ctrl_cmd;
    std::string res_str;
    static bool s_firstIn = false;
    switch (cmd) {
    case kStopSts:
        break;
    case kStartSts:
        s_firstIn = true;
        GetCmdPos(emcmotStatus, emcmotConfig, cmd_pos);
        ServoProcess(cmd_pos, act_pos);
        GetActPos(act_pos);
        GetResStr(cmd_pos, act_pos, res_str);
        s_gatherSender->PushMsg(res_str);
        break;
    case kClearRes:
        if (s_firstIn) {
            s_gatherSender->ClearMsg();
            s_firstIn = false;
        }
    default:
        break;
    }
}

static int input_pin_init()
{
    int retval = 0;

    input_data_ins = (input_data*)hal_malloc(sizeof(input_data));

    //joints cmd pos 
    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->cmd_pos_x, blk_scope_id, "cmd_pos_x");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: cmd_pos_x pin creation failed\n");
        return retval;
    }

    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->cmd_pos_y, blk_scope_id, "cmd_pos_y");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: cmd_pos_y pin creation failed\n");
        return retval;
    }

    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->cmd_pos_z, blk_scope_id, "cmd_pos_z");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: cmd_pos_z pin creation failed\n");
        return retval;
    }

    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->cmd_pos_a, blk_scope_id, "cmd_pos_a");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: cmd_pos_a pin creation failed\n");
        return retval;
    }

    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->cmd_pos_b, blk_scope_id, "cmd_pos_b");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: cmd_pos_b pin creation failed\n");
        return retval;
    }

    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->cmd_pos_c, blk_scope_id, "cmd_pos_c");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: cmd_pos_c pin creation failed\n");
        return retval;
    }

    //joints feedback pos
    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->fb_pos_x, blk_scope_id, "fb_pos_x");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: fb_pos_x pin creation failed\n");
        return retval;
    }

    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->fb_pos_y, blk_scope_id, "fb_pos_y");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: fb_pos_y pin creation failed\n");
        return retval;
    }

    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->fb_pos_z, blk_scope_id, "fb_pos_z");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: fb_pos_z pin creation failed\n");
        return retval;
    }

    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->fb_pos_a, blk_scope_id, "fb_pos_a");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: fb_pos_a pin creation failed\n");
        return retval;
    }

    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->fb_pos_b, blk_scope_id, "fb_pos_b");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: fb_pos_b pin creation failed\n");
        return retval;
    }

    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->fb_pos_c, blk_scope_id, "fb_pos_c");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: fb_pos_c pin creation failed\n");
        return retval;
    }

    //tcp cmd and actual pos
    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->cmd_pos_tcp, blk_scope_id, "cmd_pos_tcp");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: cmd_pos_tcp pin creation failed\n");
        return retval;
    }

    retval = hal_pin_float_newf(HAL_IN, &input_data_ins->fb_pos_tcp, blk_scope_id, "fb_pos_tcp");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: fb_pos_tcp pin creation failed\n");
        return retval;
    }

    retval = hal_pin_s32_newf(HAL_IN, &input_data_ins->ctrl_cmd, blk_scope_id, "ctrl_cmd");
    if (retval != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: fb_pos_tcp pin creation failed\n");
        return retval;
    }

    retval = hal_export_funct("blk_scope_catch_cycle", catch_cycle, 0	/* arg
	 */ , 1 /* uses_fp */ , 0 /* reentrant */ , blk_scope_id);

    if (retval < 0) {
        rtapi_print_msg(RTAPI_MSG_ERR,
	    "RING: failed to export gather function function\n");
        return -1;
    }

    return 0;
}

static void create_servo_strategy()
{
    ServoStrategyFactory factory;
    s_servoStragegy[kX] = factory.CreateStrategy("X");
    s_servoStragegy[kY] = factory.CreateStrategy("Y");
    s_servoStragegy[kZ] = factory.CreateStrategy("Z");
    s_servoStragegy[kA] = factory.CreateStrategy("A");
    s_servoStragegy[kB] = factory.CreateStrategy("B");
    s_servoStragegy[kC] = factory.CreateStrategy("C");
    
    for (int axis = 0; axis < kAxisNum; axis++) {
        s_servoStragegy[axis]->Initialize();
    }
}

int rtapi_app_main() {
    blk_scope_id = hal_init("blk_scope");
	rtapi_print_msg(RTAPI_MSG_ERR, "RING: Hello blk_scope V1\n");
    if (blk_scope_id < 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: ERROR: hal_init() failed\n");
        return -1;
    }
    
    rtapi_print_msg(RTAPI_MSG_ERR, "RING: Try to init input pins\n");
    if (input_pin_init() != 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: input_pins init error\n");
    }
    else {
        rtapi_print_msg(RTAPI_MSG_ERR, "RING: Inited \n");
    }

    s_gatherSender = new GatherSender();
    create_servo_strategy();
    
    hal_ready(blk_scope_id);
    return 0;
}

void rtapi_app_exit()
{
    s_gatherSender = nullptr;
    for (int axis = kX; axis < kAxisNum; axis++) {
        free (s_servoStragegy[axis]);
        s_servoStragegy[axis] = nullptr;
    }
	rtapi_print_msg(RTAPI_MSG_ERR, "RING:Bye blk_scope");
    hal_exit(blk_scope_id);
    return;
}