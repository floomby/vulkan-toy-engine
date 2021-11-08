#pragma once

#include "utilities.hpp"

namespace Pathgen {
    enum class Kind {
        OMNI,
        COUNT
    };
    // yaw, pitch, roll
    template<Kind K> std::array<glm::vec3, 2> move(const float maxSpeed, const float acceleration, const glm::vec3& dOmega, const glm::vec3& heading,
        const glm::vec3& position, const glm::vec3& dP, const glm::vec3& dH, const glm::vec3& destination);
    std::array<glm::vec3, 2> move<OMNI>(const float maxSpeed, const float acceleration, const glm::vec3& dOmega, const glm::vec3& heading,
        const glm::vec3& position, const glm::vec3& dP, const glm::vec3& dH, const glm::vec3& destination) {
        
        // For the simplest case (move to a single point without following commands queued) I have formalized the problem
/*

Find $dP(t)$ such that $P(\tau)$ is the destination where
\begin{equation}
P(\tau) = \int_{0}^{\tau}dP(t)dt + P_0 
\end{equation}
which minimizes $\tau$ and $dP(\tau) = 0$ and $|dP(a)| < a_m \: \forall \: a \in [0,\tau]$ for arbitrary $a_m > 0$

*/

        // tbh for moving through a point to another command I haven't yet formalized it
    }
};