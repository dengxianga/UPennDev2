#include "gz_comms_manager.h"

// gz_comms_manager.so : gazebo plugin for joint control and proprioception
////////////////////////////////////////////////////////////////////////////////

// Mike Hopkins 3/20/13

namespace gazebo
{
  gz_comms_manager::gz_comms_manager()
  {
  }

  gz_comms_manager::~gz_comms_manager()
  {
    event::Events::DisconnectWorldUpdateBegin(this->update_connection);
    event::Events::DisconnectStop(this->reset_connection);
  }

  void gz_comms_manager::Load(physics::ModelPtr _parent, sdf::ElementPtr _sdf)
  {
    // initialize pointers
    this->model = _parent;
    this->world = this->model->GetWorld(); 

    // load config parameters
    Config config;
    this->joint_max =
      config.get_int("comms_manager.joint_max");
    this->joint_damping = 
      config.get_double_vector("comms_manager.joint_damping");
    this->p_gain_constant =
      config.get_double_vector("comms_manager.p_gain_constant");
    this->i_gain_constant =
      config.get_double_vector("comms_manager.i_gain_constant");
    this->d_gain_constant =
      config.get_double_vector("comms_manager.d_gain_constant");
    this->d_break_freq =
      config.get_double_vector("comms_manager.d_break_freq");
    this->p_gain_default =
      config.get_double_vector("comms_manager.p_gain_default");
    this->i_gain_default =
      config.get_double_vector("comms_manager.i_gain_default");
    this->d_gain_default =
      config.get_double_vector("comms_manager.d_gain_default");
    this->physics_time_step =
      config.get_double("world_manager.physics_time_step");

    // initialize joints
    std::vector<std::string> joint_id;
    joint_id = config.get_string_vector("joint.id");
    joint_id.resize(this->joint_max);
    this->joints.resize(joint_id.size());
    for (int i = 0; i < this->joints.size(); i++)
    { 
      this->joint_index.push_back(i);
      this->joints[i] = this->model->GetJoint(joint_id[i]);
      if (!this->joints[i])
      { 
	gzerr << "Error: joint [" << joint_id[i] << "] not present\n";
	return;
      }

      this->dcm.joint_enable[i] = 1; 
      this->dcm.joint_p_gain[i] = this->p_gain_default[i];
      this->dcm.joint_i_gain[i] = this->i_gain_default[i];
      this->dcm.joint_d_gain[i] = this->d_gain_default[i];
      this->dcm.joint_position[i] = 0;
      this->dcm.joint_velocity[i] = 0;
      this->dcm.joint_force[i] = 0;
    }

    // initialize joint indexes
    this->l_ankle_index = -1;
    this->r_ankle_index = -1;
    this->l_wrist_index = -1;
    this->r_wrist_index = -1;
    this->l_gripper_index = -1;
    this->r_gripper_index = -1;
    for (int i = 0; i < joint_id.size(); i++)
    {
      if (joint_id[i] == "l_ankle_roll")
        this->l_ankle_index = i; 
      else if (joint_id[i] == "r_ankle_roll")
        this->r_ankle_index = i; 
      else if (joint_id[i] == "l_wrist_roll")
        this->l_wrist_index = i; 
      else if (joint_id[i] == "r_wrist_roll")
        this->r_wrist_index = i; 
      else if (joint_id[i] == "l_gripper")
        this->l_gripper_index = i;
      else if (joint_id[i] == "r_gripper")
        this->r_gripper_index = i;
    }

    // initialize gripper slave joints
    if (l_gripper_index > -1) 
    {
      this->joint_index.push_back(l_gripper_index);
      this->joints.push_back(this->model->GetJoint("l_gripper_slave"));
      if (!this->joints[this->joints.size() - 1])
      { 
	gzerr << "Error: l_gripper_slave joint not present\n";
	return;
      }
    }
    if (r_gripper_index > -1) 
    {
      this->joint_index.push_back(r_gripper_index);
      this->joints.push_back(this->model->GetJoint("r_gripper_slave"));
      if (!this->joints[this->joints.size() - 1])
      { 
	gzerr << "Error: r_gripper_slave joint not present\n";
	return;
      }
    }

    // intialize controllers
    this->initialize_controllers();

    // initialize contact sensors
    this->l_foot_link_name = "l_foot";
    this->l_foot_contact_sensor_name = "l_foot_contact_sensor";
    this->l_foot_contact_sensor = 
      boost::dynamic_pointer_cast<sensors::ContactSensor>(
        sensors::SensorManager::Instance()->GetSensor(
          this->world->GetName()
          + "::" + this->model->GetScopedName()
          + "::" + this->l_foot_link_name
          + "::" + this->l_foot_contact_sensor_name
        )
      );
    if (!this->l_foot_contact_sensor)
      gzerr << "l_foot_contact_sensor not found\n" << "\n";

    this->r_foot_link_name = "r_foot";
    this->r_foot_contact_sensor_name = "r_foot_contact_sensor";
    this->r_foot_contact_sensor = 
      boost::dynamic_pointer_cast<sensors::ContactSensor>(
        sensors::SensorManager::Instance()->GetSensor(
          this->world->GetName()
          + "::" + this->model->GetScopedName()
          + "::" + this->r_foot_link_name
          + "::" + this->r_foot_contact_sensor_name
        )
      );
    if (!this->r_foot_contact_sensor)
      gzerr << "r_foot_contact_sensor not found\n" << "\n";

    // initialize imu
    this->imu_link_name = "torso";
    this->imu_sensor_name = "imu_sensor";
    this->imu_sensor = 
      boost::dynamic_pointer_cast<sensors::ImuSensor>(
        sensors::SensorManager::Instance()->GetSensor(
          this->world->GetName()
          + "::" + this->model->GetScopedName()
          + "::" + this->imu_link_name
          + "::" + this->imu_sensor_name
        )
      );
    if (!this->imu_sensor)
      gzerr << "imu_sensor not found\n";

    // initialize callback functions
    if (this->l_foot_contact_sensor)
    {
      this->l_foot_contact_connection = this->l_foot_contact_sensor->ConnectUpdated(
        boost::bind(&gz_comms_manager::on_l_foot_contact, this));
    }
    if (this->r_foot_contact_sensor)
    {
      this->r_foot_contact_connection = this->r_foot_contact_sensor->ConnectUpdated(
        boost::bind(&gz_comms_manager::on_r_foot_contact, this));
    }
    this->update_connection = event::Events::Events::ConnectWorldUpdateBegin(
      boost::bind(&gz_comms_manager::update, this));
    this->reset_connection = event::Events::Events::ConnectStop(
      boost::bind(&gz_comms_manager::reset, this));
  }

  void gz_comms_manager::initialize_controllers()
  {
    // initialize dcm joint controllers
    for (int i = 0; i < this->joints.size(); i++)
    { 
      int index = this->joint_index[i];

      // set joint damping
      this->joints[i]->SetDamping(0, this->joint_damping[index]);

      // get joint limits
      double max_force = this->joints[i]->GetEffortLimit(0);
      double min_position = this->joints[i]->GetLowerLimit(0).Radian();
      double max_position = this->joints[i]->GetUpperLimit(0).Radian();
      double max_velocity = this->joints[i]->GetVelocityLimit(0);

      // initialize position pid controller
      struct pid position_pid = new_pid(this->physics_time_step);
      pid_set_setpoint_limits(&position_pid, min_position, max_position);
      if (max_force >= 0)
        pid_set_output_limits(&position_pid, -max_force, max_force);

      // initialize velocity filter
      struct filter velocity_filter = new_low_pass(
        this->physics_time_step,
        this->d_break_freq[index]
      );
      if (max_velocity >= 0)
        filter_set_output_limits(&velocity_filter, -max_velocity, max_velocity);

      // update joint structs
      if (i < joint_position_pids.size())
        this->joint_position_pids[i] = position_pid;
      else
        this->joint_position_pids.push_back(position_pid);

      if (i < joint_velocity_filters.size())
        this->joint_velocity_filters[i] = velocity_filter;
      else
        this->joint_velocity_filters.push_back(velocity_filter);
    }
  }

  void gz_comms_manager::reset()
  {
    for (int i = 0; i < this->joints.size(); i++)
    { 
      int index = this->joint_index[i];
      this->dcm.joint_enable[i] = 1; 
      this->dcm.joint_p_gain[index] = this->p_gain_default[index];
      this->dcm.joint_i_gain[index] = this->i_gain_default[index];
      this->dcm.joint_d_gain[index] = this->d_gain_default[index];
      this->dcm.joint_position[index] = 0;
      this->dcm.joint_velocity[index] = 0;
      this->dcm.joint_force[index] = 0;
    }
    this->initialize_controllers();
  }

  void gz_comms_manager::update()
  {
    // update force-torque sensors
    {
      physics::JointWrench l_wrist_wrench, r_wrist_wrench;

      unsigned int index0 = 0;
      if (l_wrist_index != -1)
        l_wrist_wrench = this->joints[l_wrist_index]->GetForceTorque(index0);
      if (r_wrist_index != -1)
        r_wrist_wrench = this->joints[r_wrist_index]->GetForceTorque(index0);

      math::Vector3 l_wrist_force = l_wrist_wrench.body2Force;
      math::Vector3 l_wrist_torque = l_wrist_wrench.body2Torque;
      math::Vector3 r_wrist_force = r_wrist_wrench.body2Force;
      math::Vector3 r_wrist_torque = r_wrist_wrench.body2Torque;

      for (int i = 0; i < 3; i++)
      {
        this->dcm.force_torque[i+12] = l_wrist_force[i];
        this->dcm.force_torque[i+15] = l_wrist_torque[i];
        this->dcm.force_torque[i+18] = r_wrist_force[i];
        this->dcm.force_torque[i+21] = r_wrist_torque[i];
      }
    }

    // update imu sensor
    if (this->imu_sensor)
    {
      math::Vector3 accel = this->imu_sensor->GetLinearAcceleration();
      math::Vector3 gyro = this->imu_sensor->GetAngularVelocity();
      math::Vector3 euler = this->imu_sensor->GetOrientation().GetAsEuler();
      for (int i = 0; i < 3; i++)
      {
        this->dcm.ahrs[i] = accel[i];
        this->dcm.ahrs[i+3] = gyro[i];
        this->dcm.ahrs[i+6] = euler[i];
      }
    }

    // update joints
    for (int i = 0; i < this->joints.size(); i++)
    {
      int index = this->joint_index[i];

      // update setpoints and sensor values
      double enable = this->dcm.joint_enable[index];
      double p_gain = this->p_gain_constant[index]*this->dcm.joint_p_gain[index];
      double i_gain = this->i_gain_constant[index]*this->dcm.joint_i_gain[index];
      double d_gain = this->d_gain_constant[index]*this->dcm.joint_d_gain[index];
      double force_setpoint = this->dcm.joint_force[index];
      double position_setpoint = this->dcm.joint_position[index];
      double velocity_setpoint = this->dcm.joint_velocity[index];
      double position_actual = this->joints[i]->GetAngle(0).Radian();
      double velocity_actual = this->joints[i]->GetVelocity(0);
      double max_force = this->joints[i]->GetEffortLimit(0);

      // update feedforward force
      double force_command = force_setpoint;

      // update position controller
      struct pid *position_pid = &joint_position_pids[i];
      pid_set_setpoint(position_pid, position_setpoint);
      pid_set_gains(position_pid,
        (enable ? p_gain : 0),
        (enable ? i_gain : 0),
        0
      );
      force_command += pid_update(position_pid, position_actual);

      // update velocity controller
      struct filter *velocity_filter = &joint_velocity_filters[i];
      double velocity_estimate = filter_update(velocity_filter, velocity_actual);
      force_command += d_gain*(velocity_setpoint - velocity_estimate);
       
      // update joint force 
      if (enable == 0)
        force_command = 0;
      else if (max_force >= 0)
        force_command = math::clamp(force_command, -max_force, max_force);
      this->joints[i]->SetForce(0, force_command);

      // update joint sensors
      this->dcm.joint_force_sensor[index] = force_command;
      this->dcm.joint_position_sensor[index] = position_actual;
      this->dcm.joint_velocity_sensor[index] = velocity_estimate;

    //gzerr << "force command " << force_command << "\n";
    }
  }

  void gz_comms_manager::on_l_foot_contact()
  {
    //update l_foot force-torque estimate
    msgs::Contacts contacts;
    contacts = this->l_foot_contact_sensor->GetContacts();
    int n_contacts = contacts.contact_size();
    int n_samples = n_contacts > 2 ? 2 : n_contacts;

    math::Vector3 l_foot_force;
    math::Vector3 l_foot_torque;
    l_foot_force.Set(0, 0, 0);
    l_foot_torque.Set(0, 0, 0);

    for (int i = n_contacts - n_samples; i < n_contacts; i++)
    {
      for (int j = 0; j < contacts.contact(i).wrench_size(); ++j)
      {
        // gzerr << j << "  Position:"
        //       << contacts.contact(n_contacts - 1).position(j).x() << " "
        //       << contacts.contact(n_contacts - 1).position(j).y() << " "
        //       << contacts.contact(n_contacts - 1).position(j).z() << "\n";
        // gzerr << "   Normal:"
        //       << contacts.contact(n_contacts - 1).normal(j).x() << " "
        //       << contacts.contact(n_contacts - 1).normal(j).y() << " "
        //       << contacts.contact(n_contacts - 1).normal(j).z() << "\n";
        // gzerr << "   Depth:" << contacts.contact(n_contacts - 1).depth(j) << "\n";
        l_foot_force += math::Vector3(
          contacts.contact(i).wrench(j).body_1_force().x(),
          contacts.contact(i).wrench(j).body_1_force().y(),
          contacts.contact(i).wrench(j).body_1_force().z());
        l_foot_torque += math::Vector3(
          contacts.contact(i).wrench(j).body_1_torque().x(),
          contacts.contact(i).wrench(j).body_1_torque().y(),
          contacts.contact(i).wrench(j).body_1_torque().z());
      }
    }

    if (n_samples > 0)
    {
      l_foot_force = l_foot_force/n_samples;
      l_foot_torque = l_foot_torque/n_samples;
    }

    // TODO rotate forces and torques from inertial to link coordinates

    for (int i = 0; i < 3; i++)
    {
      this->dcm.force_torque[i] = l_foot_force[i];
      this->dcm.force_torque[i+3] = l_foot_torque[i];
    }
  }

  void gz_comms_manager::on_r_foot_contact()
  {
    //update r_foot force-torque estimate
    msgs::Contacts contacts;
    contacts = this->r_foot_contact_sensor->GetContacts();
    int n_contacts = contacts.contact_size();
    int n_samples = n_contacts > 2 ? 2 : n_contacts;

    math::Vector3 r_foot_force;
    math::Vector3 r_foot_torque;
    r_foot_force.Set(0, 0, 0);
    r_foot_torque.Set(0, 0, 0);

    for (int i = n_contacts - n_samples; i < n_contacts; i++)
    {
      for (int j = 0; j < contacts.contact(i).wrench_size(); ++j)
      {
        // gzerr << j << "  Position:"
        //       << contacts.contact(n_contacts - 1).position(j).x() << " "
        //       << contacts.contact(n_contacts - 1).position(j).y() << " "
        //       << contacts.contact(n_contacts - 1).position(j).z() << "\n";
        // gzerr << "   Normal:"
        //       << contacts.contact(n_contacts - 1).normal(j).x() << " "
        //       << contacts.contact(n_contacts - 1).normal(j).y() << " "
        //       << contacts.contact(n_contacts - 1).normal(j).z() << "\n";
        // gzerr << "   Depth:" << contacts.contact(n_contacts - 1).depth(j) << "\n";
        r_foot_force += math::Vector3(
          contacts.contact(i).wrench(j).body_1_force().x(),
          contacts.contact(i).wrench(j).body_1_force().y(),
          contacts.contact(i).wrench(j).body_1_force().z());
        r_foot_torque += math::Vector3(
          contacts.contact(i).wrench(j).body_1_torque().x(),
          contacts.contact(i).wrench(j).body_1_torque().y(),
          contacts.contact(i).wrench(j).body_1_torque().z());
      }
    }

    if (n_samples > 0)
    {
      r_foot_force = r_foot_force/n_samples;
      r_foot_torque = r_foot_torque/n_samples;
    }

    // TODO rotate forces and torques from inertial to link coordinates

    for (int i = 0; i < 3; i++)
    {
      this->dcm.force_torque[i+6] = r_foot_force[i];
      this->dcm.force_torque[i+9] = r_foot_torque[i];
    }
  }

  // Register this plugin with the simulator
  GZ_REGISTER_MODEL_PLUGIN(gz_comms_manager)
}
