#include "gtest/gtest.h"
#include "types.hpp"
#include "drone.hpp"
#include "controller.hpp"

template <typename Derived>
bool expectEigenNear(const Eigen::MatrixBase<Derived> &mat1,const Eigen::MatrixBase<Derived> &mat2,double delta)
{
    if (mat1.rows()!=mat2.rows() && mat1.cols()!=mat2.cols())
        return false;
    Derived diff{(mat1-mat2).cwiseAbs()};
    return diff.maxCoeff() < delta;
}

TEST(QuadcopterAtEquilibrium,GivenEquilibriumInputs_DoesNotMove)
{
    dyn::Drone Quadcopter;
    double eq{0.55};
    dyn::uVec u{eq,eq,eq,eq};
    Quadcopter.sendMotorCmds(u);

    dyn::xVec expected_states;
    expected_states.setZero(dyn::STATE_SIZE,1);

    dyn::xVec actual_states{Quadcopter.getStates()};

    EXPECT_TRUE(expectEigenNear(expected_states,actual_states,1e-6));
}

TEST(QuadcopterAtEquilibrium,GivenAboveEquilibriumInputs_MovesUp)
{
    dyn::Drone Quadcopter;
    double above_eq{0.8};
    dyn::uVec u{above_eq,above_eq,above_eq,above_eq};
    for (int i{0}; i < 500; i++)
        Quadcopter.sendMotorCmds(u);

    dyn::xVec expected_states;
    expected_states.setZero(dyn::STATE_SIZE,1);
    expected_states(dyn::PZ) = 2.204978;
    expected_states(dyn::VZ) = -4.385592;

    dyn::xVec actual_states{Quadcopter.getStates()};

    EXPECT_TRUE(expectEigenNear(expected_states,actual_states,1e-6));
}

TEST(QuadcopterAtEquilibrium,GivenInputsToYawCCW_YawsCCW)
{
    dyn::Drone Quadcopter;
    double eq{0.55};
    double eq_off{0.1};
    dyn::uVec u{eq+eq_off,eq-eq_off,eq+eq_off,eq-eq_off};
    for (int i{0}; i < 500; i++)
        Quadcopter.sendMotorCmds(u);

    dyn::xVec expected_states;
    expected_states.setZero(dyn::STATE_SIZE,1);
    expected_states(dyn::RZ) = -0.408163;
    expected_states(dyn::WZ) = -0.816327;

    dyn::xVec actual_states{Quadcopter.getStates()};

    EXPECT_TRUE(expectEigenNear(expected_states,actual_states,1e-6));
}

TEST(QuadcopterAtEquilibrium,GivenInputsToYawCW_YawsCW)
{
    dyn::Drone Quadcopter;
    double eq{0.55};
    double eq_off{0.1};
    dyn::uVec u{eq-eq_off,eq+eq_off,eq-eq_off,eq+eq_off};
    for (int i{0}; i < 500; i++)
        Quadcopter.sendMotorCmds(u);

    dyn::xVec expected_states;
    expected_states.setZero(dyn::STATE_SIZE,1);
    expected_states(dyn::RZ) = 0.408163;
    expected_states(dyn::WZ) = 0.816327;

    dyn::xVec actual_states{Quadcopter.getStates()};

    EXPECT_TRUE(expectEigenNear(expected_states,actual_states,1e-6));
}

TEST(QuadcopterAtEquilibrium,GivenInputsToRoll_Rolls)
{
    dyn::Drone Quadcopter;
    double eq{0.55};
    double eq_off{0.1};
    dyn::uVec u{eq,eq-eq_off,eq,eq+eq_off};
    for (int i{0}; i < 100; i++)
        Quadcopter.sendMotorCmds(u);

    dyn::xVec expected_states;
    expected_states.setZero(dyn::STATE_SIZE,1);
    expected_states(dyn::PY) = 0.009859;
    expected_states(dyn::VY) = 0.192859;
    expected_states(dyn::PZ) = -0.000598;
    expected_states(dyn::VZ) = -0.041511;
    expected_states(dyn::RX) = 0.302882;
    expected_states(dyn::WX) = 3.028816;

    dyn::xVec actual_states{Quadcopter.getStates()};

    EXPECT_TRUE(expectEigenNear(expected_states,actual_states,1e-6));
}

TEST(QuadcopterAtEquilibrium,GivenInputsToPitch_Pitches)
{
    dyn::Drone Quadcopter;
    double eq{0.55};
    double eq_off{0.1};
    dyn::uVec u{eq+eq_off,eq,eq-eq_off,eq};
    for (int i{0}; i < 100; i++)
        Quadcopter.sendMotorCmds(u);

    dyn::xVec expected_states;
    expected_states.setZero(dyn::STATE_SIZE,1);
    expected_states(dyn::PX) = -0.009859;
    expected_states(dyn::VX) = -0.192859;
    expected_states(dyn::PZ) = -0.000598;
    expected_states(dyn::VZ) = -0.041511;
    expected_states(dyn::RY) = 0.302882;
    expected_states(dyn::WY) = 3.028816;

    dyn::xVec actual_states{Quadcopter.getStates()};

    EXPECT_TRUE(expectEigenNear(expected_states,actual_states,1e-6));
}

class ControllerTestFixture : public Controller, public ::testing::Test
{
public:
    ControllerTestFixture()
    {
        double roll{3.14/4};
        double pitch{3.14/8};
        double yaw{-3.14/6};
        this->setAttitude(roll,pitch,yaw);
    }

    void setAttitude(double roll,double pitch,double yaw)
    {
        this->m_x.block(dyn::RX,0,3,1) << roll, pitch, yaw;
    }
};

TEST_F(ControllerTestFixture,AskedToUpdateRotation_UpdatesCorrectly)
{
    this->updateRotation();

    dyn::RotMatrix expected_rotation;
    expected_rotation << 0.800292,0.587706,-0.118889,
                        -0.461765,0.477592,-0.747448,
                        -0.382499,0.653075,0.653595;

    dyn::RotMatrix actual_rotation{this->getRotation()};

    EXPECT_TRUE(expectEigenNear(expected_rotation,actual_rotation,1e-6));
}

TEST_F(ControllerTestFixture,AskedToUpdateA_UpdatesCorrectly)
{
    this->updateRotation();
    this->updateA();

    dyn::MatrixA expected_A;
    expected_A.setZero(dyn::STATE_SIZE,dyn::STATE_SIZE);
    expected_A.block(dyn::PX,dyn::VX,3,3) << 0.800292,0.587706,-0.118889,
                                            -0.461765,0.477592,-0.747448,
                                             0.382499,-0.653075,-0.653595;
    Eigen::Matrix3d identity;
    identity << 1,0,0,  0,1,0,  0,0,1;
    expected_A.block(dyn::RX,dyn::WX,3,3) = identity;
    expected_A.block(dyn::VX,dyn::VX,3,3) = identity*-0.033333;
    expected_A.block(dyn::VX,dyn::RX,3,3) << 0,-9.064005,0,
                                             6.411771,-2.652234,0,
                                             -6.406667,-2.654347,0;

    dyn::MatrixA actual_A{this->getA()};

    EXPECT_TRUE(expectEigenNear(expected_A,actual_A,1e-6));
}

TEST_F(ControllerTestFixture,AskedToDiscretizeAandB_DiscretizesCorrectly)
{
    this->updateRotation();
    this->updateA();
    this->discretizeAB();

    dyn::MatrixA expected_Ad;
    expected_Ad << 1,0,0,0.000226,-0.000425,0,0.008002,0.005876,-0.001189,0.000001,-0.000001,0,
                   0,1,0,0.000392,0.000245,0,-0.004617,0.004775,-0.007473,0.000001,0.000001,0,
                   0,0,1,0,0,0,0.003824,-0.006530,-0.006535,0,0,0,
                   0,0,0,1,0,0,0,0,0,0.01,0,0,
                   0,0,0,0,1,0,0,0,0,0,0.01,0,
                   0,0,0,0,0,1,0,0,0,0,0,0.01,
                   0,0,0,0,-0.090625,0,0.999667,0,0,0,-0.000453,0,
                   0,0,0,0.064107,-0.026518,0,0,0.999667,0,0.000321,-0.000133,0,
                   0,0,0,-0.064056,-0.026539,0,0,0,0.999667,-0.000320,-0.000133,0,
                   0,0,0,0,0,0,0,0,0,1,0,0,
                   0,0,0,0,0,0,0,0,0,0,1,0,
                   0,0,0,0,0,0,0,0,0,0,0,1;

    dyn::MatrixB expected_Bd;
    expected_Bd << 0.000026,0.000026,0.000027,0.000027,
                   0.000167,0.000166,0.000166,0.000166,
                   0.000146,0.000146,0.000146,0.000146,
                   0,-0.003786,0,0.003786,
                   0.003786,0,-0.003786,0,
                  -0.000102,0.000102,-0.000102,0.000102,
                  -0.000114,0,0.000114,0,
                  -0.000033,-0.000081,0.000033,0.000081,
                  -0.044617,-0.044503,-0.044550,-0.044664,
                   0,-0.757204,0,0.757204,
                   0.757204,0,-0.757204,0,
                  -0.020408,0.020408,-0.020408,0.020408;

    dyn::MatrixA actual_Ad{this->getAd()};
    dyn::MatrixB actual_Bd{this->getBd()};

    EXPECT_TRUE(expectEigenNear(expected_Ad,actual_Ad,1e-6));
    EXPECT_TRUE(expectEigenNear(expected_Bd,actual_Bd,1e-6));

}

TEST(Controller,GivenCurrentStates_SendsEquilibriumCommands)
{
    Controller mpc;
    dyn::xVec current_states;
    current_states.setZero(dyn::STATE_SIZE,1);

    double eq{0.55};
    dyn::uVec expected_input{eq,eq,eq,eq};

    dyn::uVec actual_input{mpc.calculateControl(current_states)};

    EXPECT_TRUE(expectEigenNear(expected_input,actual_input,1e-4));
}
