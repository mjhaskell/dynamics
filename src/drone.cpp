#include "drone.hpp"
#include <Eigen/Core>
#include "quat.hpp"

namespace dyn
{

Drone::Drone()
{
    m_p.mixer << m_p.k1,m_p.k1,m_p.k1,m_p.k1, 0,-m_p.arm_len*m_p.k1,0,m_p.arm_len*m_p.k1,
            m_p.arm_len*m_p.k1,0,-m_p.arm_len*m_p.k1,0, -m_p.k2, m_p.k2, -m_p.k2, m_p.k2;
    m_states.setZero(STATE_SIZE,1);
    m_att_R = Eigen::Matrix3d::Identity();
    m_rk4.dt = .1;
}

Drone::~Drone()
{
}

void Drone::sendAttitudeCmds(const cmdVec &cmds)
{

}

void Drone::sendMotorCmds(const uVec &inputs)
{
    uVec force_tau{m_p.mixer*inputs};
    this->derivatives(m_states, force_tau, m_rk4.k1);
    this->derivatives(m_states + m_rk4.dt/2*m_rk4.k1, force_tau, m_rk4.k2);
    this->derivatives(m_states + m_rk4.dt/2*m_rk4.k2, force_tau, m_rk4.k3);
    this->derivatives(m_states + m_rk4.dt*m_rk4.k3, force_tau, m_rk4.k4);
    m_states += m_rk4.dt/6.0 * (m_rk4.k1 + 2*m_rk4.k2 + 2*m_rk4.k3 + m_rk4.k4);
}

xVec Drone::getStates() const
{
    return m_states;
}

//xVec Drone::states() const
//{
//    return m_states;
//}

void Drone::derivatives(const xVec &x,const uVec &u,xVec &k)
{
    //    posd = Rb2v*[u;v;w];
    //    pnd = posd(1);
    //    ped = posd(2);
    //    hd  = -posd(3);

    quat::Quatd q_i2b{quat::Quat<double>::from_euler(x(RX),0.3,x(RZ))};
    k.segment<3>(PX) = q_i2b.rota(x.segment<3>(VX));
    k(PZ) *= -1;

//    attd = [1 sin(phi)*tan(theta) cos(phi)*tan(theta);0 cos(phi) -sin(phi);...
//            0 sin(phi)/cos(theta) cos(phi)/cos(theta)] * [p;q;r];
//    phid   = attd(1);
//    thetad = attd(2);
//    psid   = attd(3);
    m_att_R.block<3,2>(0,1) << sin(x(RX))*tan(x(RY)),cos(x(RX))*tan(x(RY)),
                               cos(x(RX)), -sin(x(RX)),
                               sin(x(RX))/cos(x(RY)),cos(x(RX))/cos(x(RY));
    k.segment<3>(RX) = m_att_R*x.segment<3>(WX);

//      % 0.1 seems to be a decent wind disturbance
//    N_wind = 0.0; %acceleration of quad from north wind
//    E_wind = 0.0; %acceleration of quad from east wind

    double ud = x(WZ)*x(VY)-x(WY)*x(VZ) - m_p.grav*sin(x(RY)) - m_p.mu/m_p.mass*x(VX);
    double vd = x(WX)*x(VZ)-x(WZ)*x(VX) + m_p.grav*cos(x(RY))*sin(x(RX)) - m_p.mu/m_p.mass*x(RY);
    double wd = x(WY)*x(VX)-x(WX)*x(VY) + m_p.grav*cos(x(RY))*cos(x(RX)) - u(U1)/m_p.mass - m_p.mu/m_p.mass*x(VZ);

    double x_grav = - m_p.grav*sin(0.3);
    double y_grav = m_p.grav*cos(0.3)*sin(x(RX));
    double z_grav = m_p.grav*cos(0.3)*cos(x(RX));

    Eigen::Vector3d rotated_grav = q_i2b.rotp(quat::e3*m_p.grav);
    k.segment<3>(VX) = x.segment<3>(VX).cross(x.segment<3>(WX)) + rotated_grav
                       - (quat::e3*u(U1) - m_p.mu*x.segment<3>(VX))/m_p.mass;

    k.segment<3>(WX) = m_p.inertia_inv * (u.segment<3>(U2) - x.segment<3>(WX).cross(
                       m_p.inertia*x.segment<3>(WX)));
}

} //end namespace dyn