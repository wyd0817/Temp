

#include<ros/ros.h>
#include "snake_control.h"
#include <cmath>
#include <vector>

const float MIN_JOYSTICK_ON = 0.05;  // ジョイスティックの値がこれより大きければ反応する
joy_handler::JoySelectedData joystick;

RobotSpec spec(
    /* num_joint = */ 20,
    /* link_length_head [m] = */ 0.0905,
    /* link_length_body [m] = */ 0.0905,
    /* link_length_tail [m] = */ 0.0905,
    /* link_diameter [m] = */ 0.080,
    /* max_joint_angle [rad] = */ 90.0*M_PI/180.0,
    /* max_joint_angle_velocity [rad/s] = */ 80.0*M_PI/180.0,
    /* odd_joint_is_yaw =  */ true //false //true
   );

// RobotSpec spec(
//     /* num_joint = */ 30,
//     /* link_length_head [m] = */ 0.20,
//     /* link_length_body [m] = */ 0.08,
//     /* link_length_tail [m] = */ 0.20,
//     /* link_diameter [m] = */ 0.10,
//     /* max_joint_angle [rad] = */ 90.0*M_PI/180.0,
//     /* max_joint_angle_velocity [rad/s] = */ 80.0*M_PI/180.0,
//     /* odd_joint_is_yaw =  */ true
//    );

//=== static メンバ変数の定義 ===============//
//ros::Subscriber SnakeControl::sub_joy_selected_data_;
SnakeControl::GaitMode SnakeControl::gait_mode_ = SnakeControl::GAIT_MODE_CRAWLER;
SnakeControl::ControlMode SnakeControl::control_mode_ = SnakeControl::CONTROL_MODE_INIT;
double SnakeControl::loop_rate_;
double SnakeControl::sampling_time_;
//=== static メンバ変数の定義 終わり =========//

CrawlerGait SnakeControl::crawler_gait_(
    /* spec = */ spec,
    /* num_ds_link_body = */ 20,  // 関節角を計算する積分の分割数
    /* min_height = */ 0.15,
    /* max_height = */ 0.25,
    /* min_width = */ 2.0*spec.link_diameter(),
    /* max_width = */ 0.4,
    /* clearance_circle = */ 2.0*spec.link_length_body()
   );
HelicalRollingGait SnakeControl::helical_rolling_gait_(
    /* spec = */ spec,
    /* num_ds_link_body = */ 20,  // 関節角を計算する積分の分割数
    /* min_diameter = */ 0.10,
    /* max_diameter = */ 0.4,
    /* min_pitch = */ 1.5*spec.link_diameter(),
    /* max_pitch = */ 0.7
   );
FlangeGait SnakeControl::flange_gait_(
    /* spec = */ spec,
    /* num_ds_link_body = */ 20,  // 関節角を計算する積分の分割数
    /* min_diameter = */ 0.10,
    /* max_diameter = */ 0.4,
    /* min_pitch = */ 1.5*spec.link_diameter(),
    /* max_pitch = */ 0.7,
    /* max_height = */ 0.3,
    /* max_width = */ 0.3,
    /* radius_of_circle = */ spec.min_radius_body()
  );

SidewindingGait SnakeControl::sidewinding_gait_(
    /* spec = */ spec,
    /* num_ds_link_body = */ 20,  // 関節角を計算する積分の分割数
    /* min_alpha = */ 0.10,
    /* max_alpha = */ 0.4,
    /* min_l = */ 1.5*spec.link_diameter(),
    /* max_l = */ 0.7,
    /*2*spec.link_length_body()*/ 0.0905

  );

HelicalCurveGait SnakeControl::helical_curve_gait_(
    /* spec = */ spec,
    /* num_ds_link_body = */ 20,  // 関節角を計算する積分の分割数
    /* min_alpha = */ 0.10,
    /* max_alpha = */ 0.4,
    /* min_l = */ 1.5*spec.link_diameter(),
    /* max_l = */ 0.7,
    /*2*spec.link_length_body()*/ 0.0905

  );

LateralRollingGait SnakeControl::lateral_rolling_gait_(
    /* spec = */ spec,
    /* num_ds_link_body = */ 20,  // 関節角を計算する積分の分割数
    /* min_alpha = */ 0.10,
    /* max_alpha = */ 0.4,
    /* min_l = */ 1.5*spec.link_diameter(),
    /* max_l = */ 0.7,
	2*spec.link_length_body()

  );

//--- activate処理用変数 ----------//
double s_head_active = 0.0;  // activateをこのsの値まで進めた
double s_head_initial = 0.0;  // activate開始時のs_headの値を記録
uint32_t max_joint_index_activated = 0;  // activateが済んだ関節番号の最大値






/** @fn
 * @brief コントローラーからのデータを受け取って動作を行う
 * @param joy_handler::JoySelectedData joy_data
 * @detail
 *  joy_selected_dataからボタン，ジョイスティックの状態を読み取りそれに応じた動作を行う
 */
void SnakeControl::CallBackOfJoySelectedData(joy_handler::JoySelectedData joy_data) {
  //ROS_INFO("js came");

  memcpy(&joystick, &joy_data, sizeof(joy_data));
  
  /*


  //=== 全モード共通 ===========================================//
  // 終了コマンド
  if (joy_data.button_select and joy_data.button_start) {  // startとselectを押した状態で
    if (joy_data.button_r1) {  // サーボリセット
      RequestJointResetAll();
      ROS_INFO("[SnakeContorl] Reset all joint.");
    } else if (joy_data.button_r2) {  // すべてFree
      RequestJointFreeAll();
      ROS_INFO("[SnakeContorl] Change all joint mode to FREE.");
    } else if (joy_data.button_l2) {  // すべてHOLD
      RequestJointHoldAll();
      ROS_INFO("[SnakeContorl] Change all joint mode to HOLD.");
    }
    // いずれも初期化モードに戻す
    if(joy_data.button_r1 or joy_data.button_r2 or joy_data.button_l2) {
      control_mode_ = CONTROL_MODE_INIT;
      ROS_INFO("[SnakeContorl] Change control mode to INIT.");
      ros::Duration(0.25).sleep();  // 連続入力しても仕方ないので少し時間を置く
    }
  }
  // SELECT押しながらは判断が必要なコマンド
  if (joy_data.button_select and not joy_data.button_start) {
    if (joy_data.button_triangle) {  // エラーをクリア
      RequestJointClearErrorAll();
      ROS_INFO("[SnakeContorl] Clear joint error");
    }
  }

  switch(control_mode_) {
   case CONTROL_MODE_INIT: { OperateInitMode(joy_data); break; }
   case CONTROL_MODE_ACTIVATE: { OperateActivateMode(joy_data); break; }
   case CONTROL_MODE_MOVE: {
    switch (gait_mode_) {
     case GAIT_MODE_CRAWLER: OperateMoveCrawler(joy_data); break;
     case GAIT_MODE_HELICAL_ROLLING: OperateMoveHelicalRolling(joy_data); break;
     case GAIT_MODE_FLANGE: OperateMoveFlange(joy_data); break;
    }  // switch (gait_mode_)
    break;
   }  // case CONTROL_MODE_MOVE
  }  // switch(control_mode_)

  */
}

//////////////////////////////////////////////////////////////////////////////////////////
void SnakeControl::OperateMoveLateralUndulation(joy_handler::JoySelectedData joy_data)
{
  std::vector<double> target_joint_angle(spec.num_joint(), 0.0);  // 値0.0で初期化;

  for (uint32_t i_joint=0; i_joint<spec.num_joint(); i_joint++) {

    double A = 1.0;
    double omega = 0.05;
    double theta = 1.0;
    static double t = 0.0;
    
    if (joystick.joy_stick_l_y_upwards > 0) {
      t = t + sampling_time_;
    }
    if (joystick.joy_stick_l_y_upwards < 0) {
      t = t - sampling_time_;
    }

    if ( (i_joint+1)%2 == (uint8_t)spec.odd_joint_is_yaw() ) {
      target_joint_angle[i_joint] = A * sin(theta * i_joint + omega * t);
    }else{
      target_joint_angle[i_joint] = 0.0;
    }
  }

  static int init_flag = 0;
  if (gait_mode_ != GAIT_MODE_LATERAL_UNDULATION){
    gait_mode_ = GAIT_MODE_LATERAL_UNDULATION;
    init_flag = 1;
  }
  if (init_flag == 1){
    //SnakeControlRequest::RequestJointActivateAll();
    //SnakeControlRequest::RequestJointSetPosition(joint_angle);
    for(int i = 0; i<spec.num_joint(); i++){
      SnakeControlRequest::RequestJointSetPositionRange(target_joint_angle, 0, i);
      ros::Duration(0.25).sleep();
    }
    //SnakeControlRequest::RequestJointReadPositionAll();
    init_flag = 0;
    return;
  }else{
    //SnakeControlRequest::RequestJointActivateAll();
    SnakeControlRequest::RequestJointSetPosition(target_joint_angle);
    //SnakeControlRequest::RequestJointReadPositionAll();
  }

}

void SnakeControl::OperateMoveStraight(joy_handler::JoySelectedData joy_data)
{

  std::vector<double> target_joint_angle(spec.num_joint(), 0.0);  // 値0.0で初期化;

  static int init_flag = 0;
  if (gait_mode_ != GAIT_MODE_STRAIGHT){
    gait_mode_ = GAIT_MODE_STRAIGHT;
    init_flag = 1;
  }
  if (init_flag == 1){
    //SnakeControlRequest::RequestJointActivateAll();
    //SnakeControlRequest::RequestJointSetPosition(joint_angle);
    for(int i = 0; i<spec.num_joint(); i++){
      SnakeControlRequest::RequestJointSetPositionRange(target_joint_angle, 0, i);
      ros::Duration(0.25).sleep();
    }
    //    SnakeControlRequest::RequestJointReadPositionAll();
    init_flag = 0;
    return;
  }else{
    //SnakeControlRequest::RequestJointActivateAll();
    SnakeControlRequest::RequestJointSetPosition(target_joint_angle);
    //SnakeControlRequest::RequestJointReadPositionAll();
  }

}

void SnakeControl::OperateMoveTest(joy_handler::JoySelectedData joy_data)
{
  std::vector<double> target_joint_angle(spec.num_joint(), 0.0);  // 値0.0で初期化;

  for (uint32_t i_joint=0; i_joint<spec.num_joint(); i_joint++) {  // 関節ごとにループ
    double A = 1.0;
    double omega = 0.05;
    double theta = 1.0;
    static double t = 0.0;
    
    if (joystick.joy_stick_l_y_upwards > 0) {
      t = t + sampling_time_;
    }
    if (joystick.joy_stick_l_y_upwards < 0) {
      t = t - sampling_time_;
    }

    target_joint_angle[i_joint] = A * sin(theta * i_joint + omega * t);
      
  }

  static int init_flag = 0;
  if (gait_mode_ != GAIT_MODE_TEST){
    gait_mode_ = GAIT_MODE_TEST;
    init_flag = 1;
  }
  if (init_flag == 1){
    //SnakeControlRequest::RequestJointActivateAll();
    //SnakeControlRequest::RequestJointSetPosition(joint_angle);
    for(int i = 0; i<spec.num_joint(); i++){
      SnakeControlRequest::RequestJointSetPositionRange(target_joint_angle, 0, i);
      ros::Duration(0.25).sleep();
    }
    //    SnakeControlRequest::RequestJointReadPositionAll();
    init_flag = 0;
    return;
  }else{
    //SnakeControlRequest::RequestJointActivateAll();
    SnakeControlRequest::RequestJointSetPosition(target_joint_angle);
    //    SnakeControlRequest::RequestJointReadPositionAll();
  }

}

void SnakeControl::OperateMoveSidewinding(joy_handler::JoySelectedData joy_data)
{

  const double PSI_CHANGING_SPEED = 0.1;
  if(joy_data.joy_stick_l_y_upwards > 0){
    sidewinding_gait_.add_psi(joy_data.joy_stick_l_y_upwards * PSI_CHANGING_SPEED);
  }

  if(joy_data.joy_stick_l_y_upwards < 0){
    sidewinding_gait_.add_psi(joy_data.joy_stick_l_y_upwards * PSI_CHANGING_SPEED);
  }

  const double RADIUS_CHANGING_SPEED = 0.005;
  if(joy_data.closs_key_up){
    sidewinding_gait_.add_radius(RADIUS_CHANGING_SPEED);
  }

  if(joy_data.closs_key_down){
    sidewinding_gait_.add_radius(-RADIUS_CHANGING_SPEED);
  }

  const double B_CHANGING_SPEED = 0.005;
  if(joy_data.closs_key_right){
    sidewinding_gait_.add_b(B_CHANGING_SPEED);
  }

  if(joy_data.closs_key_left){
    sidewinding_gait_.add_b(-B_CHANGING_SPEED);
  }

  std::vector<double> target_joint_angle(spec.num_joint(), 0.0);  // 値0.0で初期化;
  sidewinding_gait_.Sidewinding(spec);
  target_joint_angle = sidewinding_gait_.snake_model_param.angle;

  static int init_flag = 0;
  if (gait_mode_ != GAIT_MODE_SIDEWINDING){
    gait_mode_ = GAIT_MODE_SIDEWINDING;
    init_flag = 1;
  }
  if (init_flag == 1){
    //SnakeControlRequest::RequestJointActivateAll();
    //SnakeControlRequest::RequestJointSetPosition(joint_angle);
    for(int i = 0; i<spec.num_joint(); i++){
      SnakeControlRequest::RequestJointSetPositionRange(target_joint_angle, 0, i);
      ros::Duration(0.25).sleep();
    }
    //SnakeControlRequest::RequestJointReadPositionAll();
    init_flag = 0;
    return;
  }else{
    //SnakeControlRequest::RequestJointActivateAll();
    SnakeControlRequest::RequestJointSetPosition(target_joint_angle);
    //SnakeControlRequest::RequestJointReadPositionAll();
    //SnakeControlRequest::RequestIMURollPitchYaw(0);
  }

}

void SnakeControl::OperateMoveHelicalCurve(joy_handler::JoySelectedData joy_data)
{

  //変数操作
  helical_curve_gait_.SetDeltaT(sampling_time_);




  if(joystick.closs_key_right){
    helical_curve_gait_.x = helical_curve_gait_.x - 0.03;
  }

  if(joystick.closs_key_left){
    helical_curve_gait_.x = helical_curve_gait_.x + 0.03;
  }

  if(joystick.closs_key_up){
    helical_curve_gait_.y = helical_curve_gait_.y + 0.03;
  }

  if(joystick.closs_key_down){
    helical_curve_gait_.y = helical_curve_gait_.y - 0.03;
  }

  if(joystick.button_select){
    helical_curve_gait_.x = 0;
    helical_curve_gait_.y = 0;
  }

  // if(joystick.button_select and joystick.closs_key_left){ 
  // 	helical_curve_gait_.cop_time = 1;
  // }

  // if(joystick.button_select and joystick.closs_key_down){ 
  // 	helical_curve_gait_.cop_time = 0;
  // }


   if(joystick.button_select and joystick.closs_key_right){
     helical_curve_gait_.lowgain = 1;
     ROS_INFO("low_gain");
   }
   else{
     helical_curve_gait_.lowgain = 0;
  //   ROS_INFO("default_gain");
  }





  if(joystick.button_triangle){
    helical_curve_gait_.theta_off_0 = helical_curve_gait_.theta_off_0 + 0.01;
  }

  if(joystick.button_cross){
    helical_curve_gait_.theta_off_0 = helical_curve_gait_.theta_off_0 - 0.01;
  }


  if(joystick.button_r1){
    helical_curve_gait_.sss = helical_curve_gait_.sss + (joystick.joy_stick_l_y_upwards*helical_curve_gait_.r*helical_curve_gait_.dt)/tan(helical_curve_gait_.al);
  }

  if(joystick.button_l1){
    helical_curve_gait_.s4 = helical_curve_gait_.s4 + (joystick.joy_stick_l_y_upwards*helical_curve_gait_.r*helical_curve_gait_.dt)/tan(helical_curve_gait_.al);
  }

  if(joystick.button_r2){
    helical_curve_gait_.s5 = helical_curve_gait_.s5 + (joystick.joy_stick_l_y_upwards*helical_curve_gait_.r*helical_curve_gait_.dt)/tan(helical_curve_gait_.al);
  }

  if(joystick.button_l2){
    helical_curve_gait_.s6 = helical_curve_gait_.s6 + (joystick.joy_stick_l_y_upwards*helical_curve_gait_.r*helical_curve_gait_.dt)/tan(helical_curve_gait_.al);
  }

  if(helical_curve_gait_.sss<0){
    helical_curve_gait_.sss=0;
  }

  if(helical_curve_gait_.s4<0){
    helical_curve_gait_.s4=0;
  }

  if(helical_curve_gait_.s5<0){
    helical_curve_gait_.s5=0;
  }

  if(helical_curve_gait_.s6<0){
    helical_curve_gait_.s6=0;
  }


  if(helical_curve_gait_.sss > helical_curve_gait_.dss*spec.num_joint()){
    helical_curve_gait_.sss = helical_curve_gait_.dss*spec.num_joint();
  }   


  if(helical_curve_gait_.sss<helical_curve_gait_.s4){
    helical_curve_gait_.s4=helical_curve_gait_.sss;
  }

  if(helical_curve_gait_.s4<helical_curve_gait_.s5){
    helical_curve_gait_.s5=helical_curve_gait_.s4;
  }

  if(helical_curve_gait_.s5<helical_curve_gait_.s6){
    helical_curve_gait_.s6=helical_curve_gait_.s5;
  }


  
  //捻転速度入力
  if(joystick.joy_stick_l_y_upwards != 0){
    //helical_curve_gait_.InputOmega( joystick.joy_stick_l_y_upwards );
    helical_curve_gait_.InputOmega( 1.5 * joystick.joy_stick_l_y_upwards );
  }

  //helical_curve_gait_.ChangePsi0( helical_curve_gait_.psi_0 += joystick.joy_stick_l_y_upwards/100);  
  //半径変更(ピッチをそのままにするようにalphaも変える)
  if(joystick.joy_stick_r_y_upwards != 0){
    double delta_R = joystick.joy_stick_r_y_upwards/5000;
    helical_curve_gait_.ChangeAlpha( atan2( (helical_curve_gait_.R * tan(helical_curve_gait_.al) ), (helical_curve_gait_.R + delta_R) ) );
    helical_curve_gait_.ChangeR( helical_curve_gait_.R + delta_R );
  }
  //ピッチ変更
  if(joystick.joy_stick_r_x_rightwards != 0){
    helical_curve_gait_.ChangeAlpha( helical_curve_gait_.al + joystick.joy_stick_r_x_rightwards/1000 );
  }
  //helical_curve_gait_.ChangeThetaOffset0( helical_curve_gait_.theta_off_0 += joystick.joy_stick_r_x_rightwards/1000 );

  //計算する
  std::vector<double> target_joint_angle(spec.num_joint(), 0.0);  // 値0.0で初期化;
  target_joint_angle = helical_curve_gait_.CalcJointAngle2();

  static int init_flag = 0;
  if (gait_mode_ != GAIT_MODE_HELICAL_CURVE){
    gait_mode_ = GAIT_MODE_HELICAL_CURVE;
    init_flag = 1;
  }
  if (init_flag == 1){
    //SnakeControlRequest::RequestJointActivateAll();
    //SnakeControlRequest::RequestJointSetPosition(joint_angle);
        for(int i = 0; i<spec.num_joint(); i++){
//     for(int i = 0; i<spec.num_joint()-2; i++){
      SnakeControlRequest::RequestJointSetPositionRange(target_joint_angle, 0, i);
      ros::Duration(0.25).sleep();
    }

//     SnakeControlRequest::RequestJointFree(spec.num_joint()-2);  //最後尾リンクパッシブ
  //   SnakeControlRequest::RequestJointFree(spec.num_joint()-1);  //最後尾リンクパッシブ

     //     SnakeControlRequest::RequestJointReadPositionAll();


    init_flag = 0;
    return;
  }else{
    //SnakeControlRequest::RequestJointActivateAll();
        SnakeControlRequest::RequestJointSetPosition(target_joint_angle);
    //SnakeControlRequest::RequestJointSetPositionRange(target_joint_angle, 0,spec.num_joint()-2 );
   //SnakeControlRequest::RequestJointSetPositionRange(target_joint_angle, 0,spec.num_joint()-3 );
  
//   SnakeControlRequest::RequestJointFree(spec.num_joint()-2);//最後尾リンクパッシブ
//   SnakeControlRequest::RequestJointFree(spec.num_joint()-1);//最後尾リンクパッシブ
 
   //    SnakeControlRequest::RequestJointReadPositionAll();
    //SnakeControlRequest::RequestIMURollPitchYaw(0);
  }

}

void SnakeControl::OperateMoveLateralRolling(joy_handler::JoySelectedData joy_data)
{
  
  /***  LateralRollingGait の角周波数ωの調整 ***/
  const double OMEGA_CHANGING_SPEED = 0.2;
  if(joy_data.joy_stick_l_y_upwards > 0){
    lateral_rolling_gait_.add_omega(OMEGA_CHANGING_SPEED*joystick.joy_stick_l_y_upwards);
    //lateral_rolling_gait_.set_omega(OMEGA_CHANGING_SPEED*joystick.joy_stick_l_y_upwards);
  }

  if(joy_data.joy_stick_l_y_upwards < 0){
    lateral_rolling_gait_.add_omega(OMEGA_CHANGING_SPEED*joystick.joy_stick_l_y_upwards);
    //lateral_rolling_gait_.set_omega(OMEGA_CHANGING_SPEED*joystick.joy_stick_l_y_upwards);
  }
	
  /***  LateralRollingGait の振幅Aの調整 ***/
  const double A_CHANGING_SPEED = 0.001;
  if(joy_data.closs_key_up){
    lateral_rolling_gait_.add_A(A_CHANGING_SPEED);
  }

  if(joy_data.closs_key_down){
    lateral_rolling_gait_.add_A(-A_CHANGING_SPEED);
  }

  std::vector<double> target_joint_angle(spec.num_joint(), 0.0);  // 値0.0で初期化;
  //lateral_rolling_gait_.Rolling(spec);
  //target_joint_angle = lateral_rolling_gait_.snake_model_param.angle;
  target_joint_angle = lateral_rolling_gait_.Rolling2(spec);

  /*
  if(joystick.joy_stick_l_y_upwards > 0){
    rateral_rolling_gait_.vs =  rateral_rolling_gait_.vs + 0.5;
  }

  if(joystick.joy_stick_l_y_upwards < 0){
    rateral_rolling_gait_.vs =  rateral_rolling_gait_.vs - 0.5;
  }

  rateral_rolling_gait_.Rolling(spec);
  target_joint_angle = rateral_rolling_gait_.snake_model_param.angle;
  */

  static int init_flag = 0;
  if (gait_mode_ != GAIT_MODE_LATERAL_ROLLING){
    gait_mode_ = GAIT_MODE_LATERAL_ROLLING;
    init_flag = 1;
  }
  if (init_flag == 1){
    //SnakeControlRequest::RequestJointActivateAll();
    //SnakeControlRequest::RequestJointSetPosition(joint_angle);
    for(int i = 0; i<spec.num_joint(); i++){
      SnakeControlRequest::RequestJointSetPositionRange(target_joint_angle, 0, i);
      ros::Duration(0.25).sleep();
    }
    //    SnakeControlRequest::RequestJointReadPositionAll();
    init_flag = 0;
    return;
  }else{
    //SnakeControlRequest::RequestJointActivateAll();
    SnakeControlRequest::RequestJointSetPosition(target_joint_angle);
    //    SnakeControlRequest::RequestJointReadPositionAll();
  }
  
}

void SnakeControl::OperateMoveHelicalRolling2(joy_handler::JoySelectedData joy_data)
{
  static int init_flag = 0;
  if (gait_mode_ != GAIT_MODE_HELICAL_ROLLING2){
    gait_mode_ = GAIT_MODE_HELICAL_ROLLING2;
    init_flag = 0;
  }

  if (init_flag == 0) {
    // activate関係のパラメータを初期化
    max_joint_index_activated = 0;
    s_head_active = 0.0;

    helical_rolling_gait_.InitializeShape();
    s_head_initial = helical_rolling_gait_.s_head();
    init_flag = 1;
  }

  if (init_flag == 1) {
    //--- activateが最後までいってないときの動作
    if (max_joint_index_activated < spec.num_joint()-1) {
      //helical_rolling_gait_.SShiftByRatio(joy_data.joy_stick_r_y_upwards, sampling_time_);
      helical_rolling_gait_.SShiftByRatio(-1.0, sampling_time_);
      s_head_active = helical_rolling_gait_.s_head() - s_head_initial;
    }
    //--- s_head_activeが関節の位置を越えたらその関節をactivateする
    if (std::abs(s_head_active) > spec.length_from_head_to_joint().at(max_joint_index_activated)) {
      max_joint_index_activated += 1;
      // 念のため，activate済みの関節にも再度activateコマンドを送る
      for (int8_t i_joint=0; i_joint<=max_joint_index_activated; i_joint++) {
        RequestJointActivate(i_joint);
      }
      ROS_INFO("[SnakeContorl] activate joint %d", max_joint_index_activated);
    }
    //--- 有効化済みの関節には目標値を送信
    std::vector<double> target_joint_angle;
    target_joint_angle = helical_rolling_gait_.CalcJointAngle();
    RequestJointSetPositionRange(target_joint_angle, 0, max_joint_index_activated);
    //--- 全関節がactivateできている場合
    if (max_joint_index_activated == spec.num_joint()-1) {
      //--- activateモード終了 --> MOVEモードに移行
      init_flag = 2;
    }
  }

  if (init_flag == 2) {
    OperateMoveHelicalRolling(joy_data);
  }
}


/////////////////////////////////////////////////////////////////////////////////

/** @fn
 * @brief 初期化モードでの動作
 */
void SnakeControl::OperateInitMode(joy_handler::JoySelectedData joy_data) {

  //--- PSボタンでACTIVATEモードに移る
  if (joy_data.button_ps) {
    // 一つ目の関節を有効化しておく
    RequestJointActivate(0);
    ROS_INFO("[SnakeContorl] activate joint %d", 0);
    // activate関係のパラメータを初期化
    max_joint_index_activated = 0;
    s_head_active = 0.0;
    // 歩容ごとに形状を初期化
    switch(gait_mode_) {
     case GAIT_MODE_CRAWLER: {
      crawler_gait_.InitializeShape();
      s_head_initial = crawler_gait_.s_head();
      break;
     }
     case GAIT_MODE_HELICAL_ROLLING: {
      helical_rolling_gait_.InitializeShape();
      s_head_initial = helical_rolling_gait_.s_head();
      break;
     }
     case GAIT_MODE_FLANGE: {
      flange_gait_.InitializeShape();
      flange_gait_.SShiftHeadToEndOfBreagePart();
      s_head_initial = flange_gait_.s_head();
      break;
     }
    } // switch
    // 表示用に全てのtarget_poitionを0に
    PublishJointTargetPositionAllZero(spec.num_joint());
    // activateモードに変更
    control_mode_ = CONTROL_MODE_ACTIVATE;
  }

  //--- 実行する歩容を変更 ----------//
  if (joy_data.button_start and !joy_data.button_select) {
    if (joy_data.closs_key_right) {
      switch(gait_mode_) {
       case GAIT_MODE_CRAWLER: {
        gait_mode_ = GAIT_MODE_HELICAL_ROLLING;
        ROS_INFO("Change GaitMode to Helical Rolling");
        break;
       }
       case GAIT_MODE_HELICAL_ROLLING: {
        gait_mode_ = GAIT_MODE_FLANGE;
        ROS_INFO("Change GaitMode to Flange");
        break;
       }
       case GAIT_MODE_FLANGE: {
        gait_mode_ = GAIT_MODE_CRAWLER;
        ROS_INFO("Change GaitMode to Crawler");
        break;
       }
      } // switch(gait_mode_)
      ros::Duration(0.5).sleep();  // 入力しやすいように少し時間を置く
    }
  }
}

/** @fn
 * @brief Activateモードでの動作
 */
void SnakeControl::OperateActivateMode(joy_handler::JoySelectedData joy_data) {
  // 負の向きにS-SHIFTすることで先頭から順にactivate

  //--- activateが最後までいってないときの動作
  if (max_joint_index_activated < spec.num_joint()-1) {

    // ロボット先頭位置とs_head_activeを一緒にシフトさせる 右ジョイスティックを下に下げる
    if (joy_data.joy_stick_r_y_upwards < -MIN_JOYSTICK_ON) {
      switch (gait_mode_) {
      case GAIT_MODE_CRAWLER: {
        crawler_gait_.Move(joy_data.joy_stick_r_y_upwards, sampling_time_);
        s_head_active = crawler_gait_.s_head() - s_head_initial;
        break;
      }
      case GAIT_MODE_HELICAL_ROLLING: {
        helical_rolling_gait_.SShiftByRatio(joy_data.joy_stick_r_y_upwards, sampling_time_);
        s_head_active = helical_rolling_gait_.s_head() - s_head_initial;
        break;
      }
      case GAIT_MODE_FLANGE: {
        flange_gait_.SShiftByRatio(joy_data.joy_stick_r_y_upwards, sampling_time_);
        s_head_active = flange_gait_.s_head() - s_head_initial;
        break;
      }
      }  // switch
    }

    //--- s_head_activeが関節の位置を越えたらその関節をactivateする
    if (std::abs(s_head_active) > spec.length_from_head_to_joint().at(max_joint_index_activated)) {
      max_joint_index_activated += 1;
      // 念のため，activate済みの関節にも再度activateコマンドを送る
      for (int8_t i_joint=0; i_joint<=max_joint_index_activated; i_joint++) {
        RequestJointActivate(i_joint);
      }
      ROS_INFO("[SnakeContorl] activate joint %d", max_joint_index_activated);
      if (max_joint_index_activated == spec.num_joint()-1) {  // 最後までactiavteできた場合
        ROS_INFO("All Joint is activated! Press PS button to finish Activate mode, or press triangle button to reactivate.");
      }
    }
  }

  //--- 捻転 activateできてるかの動作確認に使う
  if (std::abs(joy_data.joy_stick_l_x_rightwards) > MIN_JOYSTICK_ON) {
    switch (gait_mode_) {
    case GAIT_MODE_CRAWLER: crawler_gait_.Roll(-joy_data.joy_stick_l_x_rightwards, sampling_time_); break;
    case GAIT_MODE_HELICAL_ROLLING: helical_rolling_gait_.Roll(-joy_data.joy_stick_l_x_rightwards, sampling_time_); break;
    case GAIT_MODE_FLANGE: flange_gait_.Roll(-joy_data.joy_stick_l_x_rightwards, sampling_time_); break;
    }
  }

  //--- 再activate済みの関節に再度activateコマンドを送る
  if (joy_data.button_triangle) {
    for (int8_t i_joint=0; i_joint<=max_joint_index_activated; i_joint++) {
      RequestJointActivate(i_joint);
    }
  }

  //--- 全関節がactivateできている場合
  if (max_joint_index_activated == spec.num_joint()-1) {
    //--- activateモード終了 --> MOVEモードに移行
    if (joy_data.button_ps) {
      control_mode_ = CONTROL_MODE_MOVE;
      ROS_INFO("[SnakeContorl] Finish ACTIVATE mode and go to MOVE mode.");
    }
  }

  //--- 有効化済みの関節には目標値を送信
  std::vector<double> target_joint_angle;
  switch (gait_mode_) {
  case GAIT_MODE_CRAWLER: target_joint_angle = crawler_gait_.CalcJointAngle(); break;
  case GAIT_MODE_HELICAL_ROLLING: target_joint_angle = helical_rolling_gait_.CalcJointAngle(); break;
  case GAIT_MODE_FLANGE: target_joint_angle = flange_gait_.CalcJointAngle(); break;
  }
  RequestJointSetPositionRange(target_joint_angle, 0, max_joint_index_activated);
}

/** @fn
 * @brief CrawlerGaitの操作用関数
 */
void SnakeControl::OperateMoveCrawler(joy_handler::JoySelectedData joy_data) {
  bool flag_pub_joint_target_position = false;

  //--- 形状変化
  if (not joy_data.button_select and joy_data.button_start) {
    // 高さ
    const double HEIGHT_CHANGING_SPEED = 0.025;  // [m/sec] 高さの変化速さ
    if (joy_data.closs_key_up) {
      crawler_gait_.add_height(HEIGHT_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    } else if (joy_data.closs_key_down) {
      crawler_gait_.add_height(-HEIGHT_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    }
    // 幅
    const double WIDTH_CHANGING_SPEED = 0.05;  // [m/sec] 幅の変化速さ
    if (joy_data.closs_key_left) {
      crawler_gait_.add_width(WIDTH_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;

    } else if (joy_data.closs_key_right) {
      crawler_gait_.add_width(-WIDTH_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    }

    // ひっくり返す
    const double REVERSE_CHANGING_SPEED = 0.5;  // [/sec] ひっくり返す動作の変化速さ
    if (std::abs(joy_data.joy_stick_l_y_upwards) > MIN_JOYSTICK_ON) {
      crawler_gait_.add_reverse_ratio(-joy_data.joy_stick_l_y_upwards*REVERSE_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    }
  } else if (not joy_data.button_select and not joy_data.button_start) {
   //--- 移動
    // 直進
    if (std::abs(joy_data.joy_stick_r_y_upwards) > MIN_JOYSTICK_ON) {
      crawler_gait_.Move(joy_data.joy_stick_r_y_upwards, sampling_time_);
      flag_pub_joint_target_position = true;
      ROS_INFO("[CircularCrawler] go straight at %4.3f", joy_data.joy_stick_r_y_upwards);
    }

    // 旋回
    const double CURVATURE_TURN_CHANGING_SPEED = 0.25;  // [/sec] 曲率変更の変化速さ
    double sgn_turn = 1.0;
    if(crawler_gait_.is_reversed()) sgn_turn = -1.0;
    if (joy_data.closs_key_right) {
      crawler_gait_.add_curvature_turn(-sgn_turn*CURVATURE_TURN_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    } else if (joy_data.closs_key_left) {
      crawler_gait_.add_curvature_turn(sgn_turn*CURVATURE_TURN_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    } else if (joy_data.closs_key_up or joy_data.closs_key_down) {
      crawler_gait_.close_curvature_turn_to_zero(CURVATURE_TURN_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    }

    // 捻転
    if (std::abs(joy_data.joy_stick_r_x_rightwards) > MIN_JOYSTICK_ON) {
      crawler_gait_.Roll(-joy_data.joy_stick_r_x_rightwards, sampling_time_);
      flag_pub_joint_target_position = true;
      ROS_INFO("[CircularCrawler] rolling at %4.3f", joy_data.joy_stick_r_x_rightwards);
    }
  }

  // 必要な時だけ目標関節角を送信
  if (flag_pub_joint_target_position) {
    RequestJointSetPosition(crawler_gait_.CalcJointAngle());
  }
}

/** @fn
 * @brief HelicalRollingGaitの操作用関数
 */
void SnakeControl::OperateMoveHelicalRolling(joy_handler::JoySelectedData joy_data) {
  bool flag_pub_joint_target_position = false;
  //--- 形状変化
  if (not joy_data.button_select and joy_data.button_start) {
    // ピッチ
    const double PITCH_CHANGING_SPEED = 0.025;  // [m/sec] pitchの変化速さ
    if (joy_data.closs_key_up) {
      helical_rolling_gait_.add_pitch(PITCH_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    } else if (joy_data.closs_key_down) {
      helical_rolling_gait_.add_pitch(-PITCH_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    }
    // 直径
    const double DIAMETER_CHANGING_SPEED = 0.020;  // [m/sec]  diameterの変化速さ
    if (joy_data.closs_key_left) {
      helical_rolling_gait_.add_diameter(DIAMETER_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    } else if (joy_data.closs_key_right) {
      helical_rolling_gait_.add_diameter(-DIAMETER_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    }
  } else if (not joy_data.button_select and not joy_data.button_start) {
    // 捻転
    if (std::abs(joy_data.joy_stick_r_y_upwards) > MIN_JOYSTICK_ON) {
      helical_rolling_gait_.Roll(joy_data.joy_stick_r_y_upwards, sampling_time_);
      flag_pub_joint_target_position = true;
      ROS_INFO("[HelicalRolling] rolling at %4.3f", joy_data.joy_stick_r_y_upwards);
    }
  }

  // 必要な時だけ目標関節角を送信
  if (flag_pub_joint_target_position) {
    RequestJointSetPosition(helical_rolling_gait_.CalcJointAngle());
  }
}

/** @fn
 * @brief MoveFlangeGaitの操作用関数
 */
void SnakeControl::OperateMoveFlange(joy_handler::JoySelectedData joy_data) {
  bool flag_pub_joint_target_position = false;
  //--- 形状変化
  if (not joy_data.button_select and joy_data.button_start) {
    // ピッチ
    const double PITCH_CHANGING_SPEED = 0.025;  // [m/sec] pitchの変化速さ
    if (joy_data.closs_key_up) {
      flange_gait_.add_pitch(PITCH_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    } else if (joy_data.closs_key_down) {
      flange_gait_.add_pitch(-PITCH_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    }
    // 直径
    const double DIAMETER_CHANGING_SPEED = 0.020;  // [m/sec] diameterの変化速さ
    if (joy_data.closs_key_left) {
      flange_gait_.add_diameter(DIAMETER_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;

    } else if (joy_data.closs_key_right) {
      flange_gait_.add_diameter(-DIAMETER_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    }
  } else if (not joy_data.button_select and not joy_data.button_start) {
    //--- 移動
    // 捻転
    if (std::abs(joy_data.joy_stick_r_y_upwards) > MIN_JOYSTICK_ON) {
      flange_gait_.Roll(joy_data.joy_stick_r_y_upwards, sampling_time_);
      flag_pub_joint_target_position = true;
      ROS_INFO("[Flange] rolling at %4.3f", joy_data.joy_stick_r_y_upwards);
    }

    // 乗り越え動作
    if (std::abs(joy_data.joy_stick_l_y_upwards) > MIN_JOYSTICK_ON) {
      flange_gait_.Stride(joy_data.joy_stick_l_y_upwards, sampling_time_);
      flag_pub_joint_target_position = true;
      ROS_INFO("[Flange] striding at %4.3f", joy_data.joy_stick_l_y_upwards);
    }

    //--- 橋部の形状変化
    // 橋部のwidth
    const double WIDTH_CHANGING_SPEED = 0.025;  // [m/sec] 橋部の幅の変化速さ
    if (joy_data.closs_key_up) {
      flange_gait_.add_width(WIDTH_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    } else if (joy_data.closs_key_down) {
      flange_gait_.add_width(-WIDTH_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    }

    // 橋部のheight
    const double HEIGHT_CHANGING_SPEED = 0.025;  // [m/sec] 橋部の高さの変化速さ
    if (joy_data.closs_key_left) {
      flange_gait_.add_height(HEIGHT_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    } else if (joy_data.closs_key_right) {
      flange_gait_.add_height(-HEIGHT_CHANGING_SPEED*sampling_time_);
      flag_pub_joint_target_position = true;
    }
  }

  // 必要な時だけ目標関節角を送信
  if (flag_pub_joint_target_position) {
    RequestJointSetPosition(flange_gait_.CalcJointAngle());
  }
}
