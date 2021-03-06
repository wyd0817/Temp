/**
 * @file helical_rolling_gait.h
 * @brief 螺旋捻転推進のためのクラス
 * @author Tatsuya TAKEMORI
 * @date 2016/08/13
 * @detail
 */

#ifndef SNAKE_CONTROL_SRC_HELICAL_CURVE_GAIT_H_
#define SNAKE_CONTROL_SRC_HELICAL_CURVE_GAIT_H_

#include <ros/ros.h>
#include <vector>

#include "robot_spec.h"
#include "simple_shape_connection.h"

#include <stdint.h>
#include "std_msgs/Float64.h"
#include "std_msgs/String.h"

/** @fn
 * @brief 横うねりのクラス
 * @detail
 *  必ずしもその必要はないが，SimpleShapeConnectionの枠組みで作る
 *  一つの螺旋がずっとループする
 */

class HelicalCurveGait : public SimpleShapeConnection {
 public:

  virtual ~HelicalCurveGait(){}

 HelicalCurveGait(RobotSpec spec,
		  uint32_t num_ds_link_body,
		  double min_alpha,
		  double max_alpha,
		  double min_l,
		  double max_l,
		  double ds
		  ) : SimpleShapeConnection(spec, num_ds_link_body) {

    /* min_alpha_ = min_alpha; */
    /* max_alpha_ = max_alpha; */
    /* min_l_ = min_l; */
    /* max_l_ = max_l; */
    /* ds_new = ds; */
    /* pre_s = 0; */
    /* s = 0; */
    /* gait_name_ = "Helical_Curve"; */
    /* InitializeShape(); */
		
    /* //dss = 0.0905;	//highpower */
    /* dss = 0.08;	//middle */
     dss = 0.0905;
    /* //r = 0.035;	//highpower */
    /* r = 0.05;	//middle */
    /* dth = M_PI/30; */
    /* beta = 0; */
    /* t = 0; */
    /* to = 0;	//theta_offset */
    /* ppsi = 0; */
    /* vs = 0; */
    /* taui = 0; */
    /* kappai = 0; */
    /* kappa_p = 0; */
    /* kappa_y = 0; */
    /* mode=0; */
    /* ppsi_old = 0; */
    /* cs = 0; */
    /* A=0; */
    /* B=0; */
    /* C=0; */
    /* b=0; */
    /* j=1; */


//    R = 0.0655;
//    R = 0.0780;  //250A用
//    R = 0.0550;  //200A用
//    R = 0.0580;  //200A用
    R = 0.0600;  //200A用

//    R0 = 0.3;
//    R0 = 0.2;
    //    al = (2*M_PI)/8;	
    //al = (2*M_PI)/6;	
//    al = (2*M_PI)/7;	//201611_ImPACT
	
    al = (89*M_PI)/314; //3巻	

    dt = 0.010;   // サンプリングタイム[sec]
    psi_0 = 0.0;
    theta_off_0 = 0.0;
    sss = 0.0;
    s4 = 0.0;
    s5 = 0.0;
    s6 = 0.0;
    top_curve_joint = 0.0;
    end_curve_joint = 0.0;
    pre_top_curve_joint = 0.0;
    pre_end_curve_joint = 0.0;
    lowgain = 0.0;
    cop_time = 0.0;
    count=0.0;
    abs_count=0.0;
    pre_abs_count=0.0; 

    x = 0;
    y = 0;


    r = 0.035;
  }


  //////////////////////////////////////
  std::vector<double> CalcJointAngle2();
  double psi_0;
  //void ChangePsi0(double psi_){ psi_0 = psi_; }
  double theta_off_0;
  void ChangeThetaOffset0(double theta_off_){
    theta_off_0 = theta_off_; 
  }
  double dt;                              // サンプリングタイム10[msec]
  void SetDeltaT(double delta_t_){
    dt = delta_t_; 
  }
  double R;	//螺旋半径
  void ChangeR(double R_){ 
    if(R_ > 0.0001){
      R = R_;
    }else{
      R = 0.0001;
    }
    ROS_INFO("R=%f", R);
  }
  double R0;	//螺旋が全体的に曲がる半径
  void ChangeR0(double R0_){ R0 = R0_;}
  double al;	//螺旋のピッチ
  void ChangeAlpha(double al_){ 
    if(al_ > 0.000){
      al = al_;
    }else{
      al = 0.0;
    }
    ROS_INFO("alpha=%f", al);
  }
  double sss;
  void ChangeThetaSSS(double sss_){
    sss = sss_;
  }
  double s4;
  void ChangeThetaS4(double s4_){
    s4 = s4_;
  }
  double s5;
  void ChangeThetaS5(double s5_){
    s5 = s5_;
  }
  double s6;
  void ChangeThetaS6(double s6_){
    s6 = s6_;
  }
  double omega; //捻転角速度(-1〜1) [rad/sec]
  void InputOmega(double omega_){
    omega = omega_;
    psi_0 = psi_0 + (omega * dt);
  }


  //double ds;                // ユニット間距離(MX106) */
   double dss; 
	 double r;		//dynamixelの半径(MX_106) */
  /* double dth; */
  /* double beta; */
  /* double t; */
  /* double to;	//theta_offset */
  /* double ppsi_old; */
  /* double ppsi; */
  /* double vs; */
  /* double taui; */
  /* double kappai; */
  /* double mode; */
  /* double kappa_p; */
  /* double kappa_y; */
  /* double cs; */
  /* double A; */
  /* double B; */
  /* double C; */
  /* double b; */
  /* double j; */

   double x;
   double y;
   double top_curve_joint;
   double end_curve_joint;
   double pre_top_curve_joint; 
   double pre_end_curve_joint; 
   double lowgain;
   double cop_time;
   double count;
   double abs_count;
   double pre_abs_count;   

  //--- 動作
  void init();
  void Winding(RobotSpec spec);
  void WindingShift();

  virtual void InitializeShape() {

  }

  /* //--- 形状パラメータ変更 */
  /* void set_alpha(double alpha); */
  /* void add_alpha(double alpha_add){ set_alpha(alpha_+alpha_add); } */
  /* void set_l(double l); */
  /* void add_l(double l_add){ set_l(l_+l_add); } */


  //void Roll(double ratio_to_max, double time_move);
  //void SShiftByRatio(double ratio_to_max, double time_move);

  //protected:
  // 制限値計算
  //double CalcMaxShiftVelocity();
  //double CalcMaxRollingVelocity();
  //void ReshapeSegmentParameter();
  // セグメントパラメータ計算用
  //static double CalcCurvatureHelix(double diameter, double pitch);
  //static double CalcTorsionHelix(double diameter, double pitch)
  //protected:
  // 形状パラメータ

  /* double l_;      // */
  /* double alpha_;     // */
  /* double alpha_s;   // */

  /* double bias_;      // バイアス */
  /* double dt_;		   // サンプリングタイム */
  /* double kp_bias_;   // 操舵バイアス比例ゲイン */
  /* double step_; */

  /* //private: */

  /* // 形状パラメータ制限用 */
  /* double min_alpha_; */
  /* double max_alpha_; */
  /* double min_l_; */
  /* double max_l_; */
  /* double bias_max_;		// 最大バイアス 単位は[rad] */
  /* double v_max_;			// 最大速度[m/s] */

  /* double s; */
  /* double v; */
  /* double ds_new; */

  /* double pre_s; */
  /* float  step_s; */

  /* /\***   サーペノイド曲線用パラメータ    ***\/ */
  /* typedef struct { */
  /*   double alpha_s;       // 生物機械工学の(3.10)式 */
  /*   double alpha;         // くねり角[rad] */
  /*   double l;             // 曲線の1/4周期の長さ[m] */

  /* }SERPENOID_CURVE; */

  /* /\***   ヘビの各関節の実行するパラメータ    ***\/ */
  /* typedef struct{      // τ(s)=0，κ_p(s)およびκ_y(s)は任意の背びれ曲線のパラメータ．横うねり推進を考慮して"bias"もパラメータとしてメンバにした． */
  /*   std::vector<double> angle; */
  /*   std::vector<double> bias; */
  /*   std::vector<double> kappa; */
  /*   std::vector<double> tau; */
  /*   std::vector<double> psi; */
  /*   std::vector<double> phi; */

  /* }SNAKE_MODEL_PARAM; */


  /* /\***   ヘビの各関節のデックのなかのデータ,  シフトするパラメータ    ***\/ */
  /* typedef struct{ */

  /*   std::vector<double> kappa_hold; */
  /*   std::vector<double> tau_hold; */
  /*   std::vector<double> bias_hold; */
  /*   std::vector<double> psi_hold; */
  /*   std::vector<double> phi_hold; */
  /*   std::vector<double> flag_hold; */

  /* }SHIFT_PARAM; */

  /* typedef struct{ */
  /*   std::vector<SHIFT_PARAM> shift_param; */

  /* }HOLD_DATA; */

  /* HOLD_DATA hold_data; */

  /* SERPENOID_CURVE     serpenoid_curve; */
  /* SNAKE_MODEL_PARAM   snake_model_param; */
  /* //std::vector<SHIFT_PARAM> shift_param; */


	
};

#endif /* SNAKE_CONTROL_SRC_HELICAL_CURVE_GAIT_H_ */
