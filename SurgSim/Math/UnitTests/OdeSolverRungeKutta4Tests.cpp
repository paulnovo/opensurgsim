// This file is a part of the OpenSurgSim project.
// Copyright 2013, SimQuest Solutions Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/// \file OdeSolverRungeKutta4Tests.cpp
/// Tests for the class OdeSolverRungeKutta4.

#include <memory>

#include <gtest/gtest.h>

#include "SurgSim/Math/OdeSolverRungeKutta4.h"
#include "SurgSim/Math/OdeSolverLinearRungeKutta4.h"
#include "SurgSim/Math/UnitTests/MockObject.h"

namespace SurgSim
{

namespace Math
{

template<class T>
void doConstructorTest()
{
	MassPoint m;
	ASSERT_NO_THROW({T solver(&m);});
}

TEST(OdeSolverRungeKutta4, ConstructorTest)
{
	{
		SCOPED_TRACE("OdeSolverRungeKutta4");
		doConstructorTest<OdeSolverRungeKutta4<MassPointState, Matrix, Matrix, Matrix, Matrix>>();
	}
	{
		SCOPED_TRACE("OdeSolverLinearRungeKutta4");
		doConstructorTest<OdeSolverLinearRungeKutta4<MassPointState, Matrix, Matrix, Matrix, Matrix>>();
	}
}

template<class T>
void doSolveTest()
{
	Vector deltaWithoutViscosity;
	Vector deltaWithViscosity;
	double dt = 1e-3;

	// Test direction correctness of a moving point under gravity (no viscosity)
	{
		MassPoint m;
		MassPointState defaultState, currentState, newState;

		T solver(&m);
		ASSERT_NO_THROW({solver.solve(dt, currentState, &newState);});
		EXPECT_EQ(defaultState, currentState);
		EXPECT_NE(defaultState, newState);

		EXPECT_FALSE(newState.getVelocities().isZero());
		EXPECT_DOUBLE_EQ(0.0, newState.getVelocities().dot(Vector3d::UnitX()));
		EXPECT_GT(0.0, newState.getVelocities().dot(Vector3d::UnitY()));
		EXPECT_DOUBLE_EQ(0.0, newState.getVelocities().dot(Vector3d::UnitZ()));

		EXPECT_FALSE(newState.getPositions().isZero());
		deltaWithoutViscosity = (newState.getPositions() - currentState.getPositions());
		EXPECT_DOUBLE_EQ(0.0, deltaWithoutViscosity.dot(Vector3d::UnitX()));
		EXPECT_GT(0.0, deltaWithoutViscosity.dot(Vector3d::UnitY()));
		EXPECT_DOUBLE_EQ(0.0, deltaWithoutViscosity.dot(Vector3d::UnitZ()));
	}

	// Test direction correctness of a moving point under gravity (viscosity)
	{
		MassPoint m(0.1);
		MassPointState defaultState, currentState, newState;

		T solver(&m);
		ASSERT_NO_THROW({solver.solve(dt, currentState, &newState);});
		EXPECT_EQ(defaultState, currentState);
		EXPECT_NE(defaultState, newState);

		EXPECT_FALSE(newState.getVelocities().isZero());
		EXPECT_DOUBLE_EQ(0.0, newState.getVelocities().dot(Vector3d::UnitX()));
		EXPECT_GT(0.0, newState.getVelocities().dot(Vector3d::UnitY()));
		EXPECT_DOUBLE_EQ(0.0, newState.getVelocities().dot(Vector3d::UnitZ()));

		EXPECT_FALSE(newState.getPositions().isZero());
		deltaWithViscosity = (newState.getPositions() - currentState.getPositions());
		EXPECT_DOUBLE_EQ(0.0, deltaWithViscosity.dot(Vector3d::UnitX()));
		EXPECT_GT(0.0, deltaWithViscosity.dot(Vector3d::UnitY()));
		EXPECT_DOUBLE_EQ(0.0, deltaWithViscosity.dot(Vector3d::UnitZ()));
	}

	EXPECT_GT(deltaWithoutViscosity.norm(), deltaWithViscosity.norm());

	// Test Runge Kutta 4 algorithm itself (without viscosity)
	{
		MassPoint m;
		MassPointState currentState, newState;
		currentState.getPositions().setLinSpaced(1.0, 3.0);
		currentState.getVelocities().setConstant(1.0);

		// Problem to solve is
		// m.a = m.g which is an ode of order 2 that can be reduced to order 1 as following:
		// y' = (x)' = (  v  ) = f(t, y)
		//      (v)  = (m.g/m)
		// In terms of (x), f(t, (x)) = (v)
		//             (v)       (v)    (g = constant)

		// Runge Kutta 4 computes y(n+1) = y(n) + 1/6.dt.(k1 + 2 * k2 + 2 * k3 + k4)
		// with k1 = f(t(n)       , y(n)            )
		// with k2 = f(t(n) + dt/2, y(n) + k1 * dt/2)
		// with k3 = f(t(n) + dt/2, y(n) + k2 * dt/2)
		// with k4 = f(t(n) + dt  , y(n) + k3 * dt  )

		struct State
		{
			State(){}
			State(const Vector& p, const Vector& v) : velocity(v), position(p){}
			Vector position;
			Vector velocity;
		};
		State yn(currentState.getPositions(), currentState.getVelocities());
		State k1, k2, k3, k4;
		State yn_plus_1;

		// 1st evaluation k1 = f(t(n)       , y(n)            )
		k1.position = yn.velocity;
		k1.velocity = m.m_gravity;

		// 2nd evaluation k2 = f(t(n) + dt/2, y(n) + k1 * dt/2)
		k2.position = yn.velocity + k1.velocity * dt / 2.0;
		k2.velocity = m.m_gravity;

		// 3rd evaluation k3 = f(t(n) + dt/2, y(n) + k2 * dt/2)
		k3.position = yn.velocity + k2.velocity * dt / 2.0;
		k3.velocity = m.m_gravity;

		// 4th evaluation k4 = f(t(n) + dt  , y(n) + k3 * dt  )
		k4.position = yn.velocity + k3.velocity * dt;
		k4.velocity = m.m_gravity;

		yn_plus_1.position = yn.position + dt / 6.0 * (k1.position + k4.position + 2.0 * (k2.position + k3.position));
		yn_plus_1.velocity = yn.velocity + dt / 6.0 * (k1.velocity + k4.velocity + 2.0 * (k2.velocity + k3.velocity));

		T solver(&m);

		ASSERT_NO_THROW({solver.solve(dt, currentState, &newState);});

		EXPECT_TRUE(currentState.getPositions().isApprox(yn.position));
		EXPECT_TRUE(currentState.getVelocities().isApprox(yn.velocity));

		EXPECT_TRUE(newState.getPositions().isApprox(yn_plus_1.position));
		EXPECT_TRUE(newState.getVelocities().isApprox(yn_plus_1.velocity));
	}

	// Test Runge Kutta 4 algorithm itself (with viscosity)
	{
		MassPoint m(0.1);
		MassPointState currentState, newState;
		currentState.getPositions().setLinSpaced(1.0, 3.0);
		currentState.getVelocities().setConstant(1.0);

		// Problem to solve is
		// m.a = m.g which is an ode of order 2 that can be reduced to order 1 as following:
		// y' = (x)' = (  v  ) = f(t, y)
		//      (v)  = (m.g/m - 0.1.v/m)
		// In terms of (x), f(t, (x)) = (v)
		//             (v)       (v)    (g - 0.1.v/m)

		// Runge Kutta 4 computes y(n+1) = y(n) + 1/6.dt.(k1 + 2 * k2 + 2 * k3 + k4)
		// with k1 = f(t(n)       , y(n)            )
		// with k2 = f(t(n) + dt/2, y(n) + k1 * dt/2)
		// with k3 = f(t(n) + dt/2, y(n) + k2 * dt/2)
		// with k4 = f(t(n) + dt  , y(n) + k3 * dt  )

		struct State
		{
			State(){}
			State(const Vector& p, const Vector& v) : velocity(v), position(p){}
			Vector position;
			Vector velocity;
		};
		State yn(currentState.getPositions(), currentState.getVelocities());
		State k1, k2, k3, k4;
		State yn_plus_1;

		// 1st evaluation k1 = f(t(n)       , y(n)            )
		k1.position = yn.velocity;
		k1.velocity = m.m_gravity - 0.1 * yn.velocity / m.m_mass;

		// 2nd evaluation k2 = f(t(n) + dt/2, y(n) + k1 * dt/2)
		k2.position = yn.velocity + k1.velocity * dt / 2.0;
		k2.velocity = m.m_gravity - 0.1 * (yn.velocity + k1.velocity * dt / 2.0) / m.m_mass;

		// 3rd evaluation k3 = f(t(n) + dt/2, y(n) + k2 * dt/2)
		k3.position = yn.velocity + k2.velocity * dt / 2.0;
		k3.velocity = m.m_gravity - 0.1 * (yn.velocity + k2.velocity * dt / 2.0) / m.m_mass;

		// 4th evaluation k4 = f(t(n) + dt  , y(n) + k3 * dt  )
		k4.position = yn.velocity + k3.velocity * dt;
		k4.velocity = m.m_gravity - 0.1 * (yn.velocity + k3.velocity * dt) / m.m_mass;

		yn_plus_1.position = yn.position + dt / 6.0 * (k1.position + k4.position + 2.0 * (k2.position + k3.position));
		yn_plus_1.velocity = yn.velocity + dt / 6.0 * (k1.velocity + k4.velocity + 2.0 * (k2.velocity + k3.velocity));

		T solver(&m);

		ASSERT_NO_THROW({solver.solve(dt, currentState, &newState);});

		EXPECT_TRUE(currentState.getPositions().isApprox(yn.position));
		EXPECT_TRUE(currentState.getVelocities().isApprox(yn.velocity));

		EXPECT_TRUE(newState.getPositions().isApprox(yn_plus_1.position));
		EXPECT_TRUE(newState.getVelocities().isApprox(yn_plus_1.velocity));
	}
}

TEST(OdeSolverRungeKutta4, SolveTest)
{
	{
		SCOPED_TRACE("OdeSolverRungeKutta4");
		doSolveTest<OdeSolverRungeKutta4<MassPointState, Matrix, Matrix, Matrix, Matrix>>();
	}
	{
		SCOPED_TRACE("OdeSolverLinearRungeKutta4");
		doSolveTest<OdeSolverLinearRungeKutta4<MassPointState, Matrix, Matrix, Matrix, Matrix>>();
	}
}

}; // Math

}; // SurgSim
