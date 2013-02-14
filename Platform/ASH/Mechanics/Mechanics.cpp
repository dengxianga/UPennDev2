#include "Mechanics.h"

_Mechanics Mechanics;

_Mechanics::_Mechanics()
{

  gravity = Vector(0, 0, -9.81);

  // coordinate transforms
  /////////////////////////////////////////////////////////////////////////

  // head
  clavicle_transform = Frame(Vector(0, 0, 0));
  neck_transform = Frame(Vector(0, 0, 0));
  head_transform = Frame(Vector(0, 0, 0));

  // l_arm
  l_shoulder_transform = Frame(Vector(0, 0.219, 0));
  l_arm_transform = Frame(Vector(0.03, 0, -0.246));
  l_forearm_transform = Frame(Vector(-0.03, 0, -0.242));
  l_hand_transform = Frame(Vector(0, 0, 0));

  // r_arm 
  r_shoulder_transform = Frame(Vector(0, -0.219, 0));
  r_arm_transform = Frame(Vector(0.03, 0, -0.246));
  r_forearm_transform = Frame(Vector(-0.03, 0, -0.242));
  r_hand_transform = Frame(Vector(0, 0, 0));

  // waist
  torso_waist_transform = Frame(Vector(-0.001, 0, 0.372));
  waist_chest_transform = Frame(Vector(0, 0, 0.144));

  // l_leg
  l_hip_transform = Frame(Vector(0, 0.097, 0));
  l_thigh_transform = Frame(Vector(0.00255, 0, -0.379025));
  l_shin_transform = Frame(Vector(0, 0, -0.3800));
  l_foot_transform = Frame(Vector(0, 0, -0.0487));

  // r_leg
  r_hip_transform = Frame(Vector(0, -0.097, 0));
  r_thigh_transform = Frame(Vector(0.00255, 0, -0.379025));
  r_shin_transform = Frame(Vector(0, 0, -0.3800));
  r_foot_transform = Frame(Vector(0, 0, -0.0487));

  // link inertias
  /////////////////////////////////////////////////////////////////////////

  // head
  neck_inertia = RigidBodyInertia(
    0,
    Vector::Zero(),
    RotationalInertia()
  );
  head_inertia = RigidBodyInertia(
    0,
    Vector::Zero(),
    RotationalInertia()
  );

  // l_arm
  l_arm_inertia = RigidBodyInertia(
    0,
    Vector::Zero(),
    RotationalInertia()
  );
  l_forearm_inertia = RigidBodyInertia(
    0,
    Vector::Zero(),
    RotationalInertia()
  );
  l_hand_inertia = RigidBodyInertia(
    0,
    Vector::Zero(),
    RotationalInertia()
  );

  // r_arm
  r_arm_inertia = RigidBodyInertia(
    0,
    Vector::Zero(),
    RotationalInertia()
  );
  r_forearm_inertia = RigidBodyInertia(
    0,
    Vector::Zero(),
    RotationalInertia()
  );
  r_hand_inertia = RigidBodyInertia(
    0,
    Vector::Zero(),
    RotationalInertia()
  );

  // waist
  chest_inertia = RigidBodyInertia(
    0,
    Vector::Zero(),
    RotationalInertia()
  );

  // l_leg
  l_thigh_inertia = RigidBodyInertia(
    2.214889327,
    Vector(-0.009015634, 0.016546555, -0.159241491),
    RotationalInertia(0.021142224, 0.024067859, 0.008757645,
                      -0.002091864, -0.001007431, -0.001750657)
  );
  l_shin_inertia = RigidBodyInertia(
    2.239063803,
    Vector(-0.025656333, -0.000475219, -0.126284289),
    RotationalInertia(0.021748921, 0.027028523, 0.010583518,
                      0.000056002, 0.004889713, 0.000035634)
  );
  l_foot_inertia = RigidBodyInertia(
    0.589284455,
    Vector(0.003616028,  0.000275946, -0.029612515),
    RotationalInertia(0.000545430, 0.001825442, 0.002054387,
                      0.000004512, -0.000179602, -0.000001187)
  );

  // r_leg
  r_thigh_inertia = RigidBodyInertia(
    2.214889327,
    Vector(-0.009015634, -0.016546555, -0.159241491),
    RotationalInertia(0.021142224, 0.024067859, 0.008757645,
                      0.002091864, -0.001007431, 0.001750657)
  );
  r_shin_inertia = RigidBodyInertia(
    2.239063803,
    Vector(-0.025656333, -0.000475219, -0.126284289),
    RotationalInertia(0.021748921, 0.027028523, 0.010583518,
                      0.000056002, 0.004889713, 0.000035634)
  );
  r_foot_inertia = RigidBodyInertia(
    0.589284455,
    Vector(0.003616028,  0.000275946, -0.029612515),
    RotationalInertia(0.000545430, 0.001825442, 0.002054387,
                      0.000004512, -0.000179602, -0.000001187)
  );

  // base
  torso_inertia = RigidBodyInertia(
    9.846994362,
    Vector(0.009821043, 0.000461078, 0.205279880),
    RotationalInertia(0.161838147, 0.120693934, 0.120189496,
                      0.000446170, -0.000151051, 0.000329275)
  );

  // kinematic chains
  /////////////////////////////////////////////////////////////////////////

  // head
  head.addSegment(Segment("clavicle", Joint(Joint::None),
    clavicle_transform));
  head.addSegment(Segment("neck", Joint(Joint::RotZ),
    neck_transform, neck_inertia));
  head.addSegment(Segment("head", Joint(Joint::RotY), 
    head_transform, head_inertia));

  // l_arm
  l_arm.addSegment(Segment("l_shoulder", Joint(Joint::None),
    l_shoulder_transform));
  l_arm.addSegment(Segment("l_shoulder_pitch", Joint(Joint::RotY)));
  l_arm.addSegment(Segment("l_shoulder_roll", Joint(Joint::RotX)));
  l_arm.addSegment(Segment("l_arm", Joint(Joint::RotZ), 
    l_arm_transform, l_arm_inertia)); 
  l_arm.addSegment(Segment("l_forearm", Joint(Joint::RotY),
    l_forearm_transform, l_forearm_inertia));
  l_arm.addSegment(Segment("l_wrist_yaw", Joint(Joint::RotZ)));
  l_arm.addSegment(Segment("l_hand", Joint(Joint::RotY),
    l_hand_transform, l_hand_inertia));

  // r_arm
  r_arm.addSegment(Segment("r_shoulder", Joint(Joint::None),
    r_shoulder_transform));
  r_arm.addSegment(Segment("r_shoulder_pitch", Joint(Joint::RotY)));
  r_arm.addSegment(Segment("r_shoulder_roll", Joint(Joint::RotX)));
  r_arm.addSegment(Segment("r_arm", Joint(Joint::RotZ), 
    r_arm_transform, r_arm_inertia)); 
  r_arm.addSegment(Segment("r_forearm", Joint(Joint::RotY),
    r_forearm_transform, r_forearm_inertia));
  r_arm.addSegment(Segment("r_wrist_yaw", Joint(Joint::RotZ)));
  r_arm.addSegment(Segment("r_hand", Joint(Joint::RotY),
    r_hand_transform, r_hand_inertia));

  // waist
  waist.addSegment(Segment("waist", Joint(Joint::None),
    torso_waist_transform));
  waist.addSegment(Segment("chest", Joint(Joint::RotZ),
    waist_chest_transform, chest_inertia));

  // l_leg
  l_leg.addSegment(Segment("l_hip", Joint(Joint::None),
    l_hip_transform));
  l_leg.addSegment(Segment("l_hip_yaw", Joint(Joint::RotZ)));
  l_leg.addSegment(Segment("l_hip_roll", Joint(Joint::RotX)));
  l_leg.addSegment(Segment("l_thigh", Joint(Joint::RotY), 
    l_thigh_transform, l_thigh_inertia)); 
  l_leg.addSegment(Segment("l_shin", Joint(Joint::RotY),
    l_shin_transform, l_shin_inertia));
  l_leg.addSegment(Segment("l_ankle_pitch", Joint(Joint::RotY)));
  l_leg.addSegment(Segment("l_foot", Joint(Joint::RotX),
    l_foot_transform, l_foot_inertia));

  // r_leg
  r_leg.addSegment(Segment("r_hip", Joint(Joint::None),
    r_hip_transform));
  r_leg.addSegment(Segment("r_hip_yaw", Joint(Joint::RotZ)));
  r_leg.addSegment(Segment("r_hip_roll", Joint(Joint::RotX)));
  r_leg.addSegment(Segment("r_thigh", Joint(Joint::RotY), 
    r_thigh_transform, r_thigh_inertia)); 
  r_leg.addSegment(Segment("r_shin", Joint(Joint::RotY),
    r_shin_transform, r_shin_inertia));
  r_leg.addSegment(Segment("r_ankle_pitch", Joint(Joint::RotY)));
  r_leg.addSegment(Segment("r_foot", Joint(Joint::RotX),
    r_foot_transform, r_foot_inertia));

  // base
  base.addSegment(Segment("base_x", Joint(Joint::TransX)));
  base.addSegment(Segment("base_y", Joint(Joint::TransY)));
  base.addSegment(Segment("base_z", Joint(Joint::TransZ)));
  base.addSegment(Segment("base_roll", Joint(Joint::RotX)));
  base.addSegment(Segment("base_pitch", Joint(Joint::RotY)));
  base.addSegment(Segment("torso", Joint(Joint::RotZ),
    Frame::Identity(), torso_inertia));

  // kinematic trees
  /////////////////////////////////////////////////////////////////////////
  body.addChain(base, "root");
  body.addChain(l_leg, "torso");
  body.addChain(r_leg, "torso");
  body.addChain(waist, "torso");
  body.addChain(l_arm, "chest");
  body.addChain(r_arm, "chest");
  body.addChain(head, "chest");

  // tree segment indices
  /////////////////////////////////////////////////////////////////////////
  body_torso_index = base.getNrOfSegments() - 1;
  body_l_foot_index = body_torso_index + l_leg.getNrOfSegments();
  body_r_foot_index = body_l_foot_index + r_leg.getNrOfSegments();
  body_chest_index = body_r_foot_index + waist.getNrOfSegments();
  body_l_hand_index = body_chest_index + l_arm.getNrOfSegments();
  body_r_hand_index = body_l_hand_index + r_arm.getNrOfSegments();
  body_head_index = body_r_hand_index + head.getNrOfSegments();

  // kinematic and dynamic solvers
  /////////////////////////////////////////////////////////////////////////

  // head
  head_fk_pos_solver = new ChainFkSolverPos_recursive(head);
  head_fk_vel_solver = new ChainFkSolverVel_recursive(head);
  head_ik_vel_solver = new ChainIkSolverVel_pinv(head);
  head_jnt_to_jac_solver = new ChainJntToJacSolver(head);
  head_dynamic_param = new ChainDynParam(head, gravity); 

  // l_arm
  l_arm_fk_pos_solver = new ChainFkSolverPos_recursive(l_arm);
  l_arm_fk_vel_solver = new ChainFkSolverVel_recursive(l_arm);
  l_arm_ik_vel_solver = new ChainIkSolverVel_pinv(l_arm);
  l_arm_jnt_to_jac_solver = new ChainJntToJacSolver(l_arm);
  l_arm_dynamic_param = new ChainDynParam(l_arm, gravity); 

  // r_arm
  r_arm_fk_pos_solver = new ChainFkSolverPos_recursive(r_arm);
  r_arm_fk_vel_solver = new ChainFkSolverVel_recursive(r_arm);
  r_arm_ik_vel_solver = new ChainIkSolverVel_pinv(r_arm);
  r_arm_jnt_to_jac_solver = new ChainJntToJacSolver(r_arm);
  r_arm_dynamic_param = new ChainDynParam(r_arm, gravity); 

  // waist
  waist_fk_pos_solver = new ChainFkSolverPos_recursive(waist);
  waist_fk_vel_solver = new ChainFkSolverVel_recursive(waist);
  waist_ik_vel_solver = new ChainIkSolverVel_pinv(waist);
  waist_jnt_to_jac_solver = new ChainJntToJacSolver(waist);
  waist_dynamic_param = new ChainDynParam(waist, gravity); 
  
  // l_leg
  l_leg_fk_pos_solver = new ChainFkSolverPos_recursive(l_leg);
  l_leg_fk_vel_solver = new ChainFkSolverVel_recursive(l_leg);
  l_leg_ik_vel_solver = new ChainIkSolverVel_pinv(l_leg);
  l_leg_jnt_to_jac_solver = new ChainJntToJacSolver(l_leg);
  l_leg_dynamic_param = new ChainDynParam(l_leg, gravity); 

  // r_leg
  r_leg_fk_pos_solver = new ChainFkSolverPos_recursive(r_leg);
  r_leg_fk_vel_solver = new ChainFkSolverVel_recursive(r_leg);
  r_leg_ik_vel_solver = new ChainIkSolverVel_pinv(r_leg);
  r_leg_jnt_to_jac_solver = new ChainJntToJacSolver(r_leg);
  r_leg_dynamic_param = new ChainDynParam(r_leg, gravity); 

  // body
  body_id_solver = new TreeIdSolver_RNE(body, gravity);
};
