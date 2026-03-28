#include "cuboid.hpp"

namespace onion::voxel
{
	Cuboid::Cuboid(const glm::vec3& center,
				   float width,
				   float height,
				   float depth,
				   const glm::vec3& facingDirection,
				   float rotation_deg)
		: m_Center(center), m_Width(width), m_Height(height), m_Depth(depth), m_FacingDirection(facingDirection),
		  m_Roration_degrees(rotation_deg)
	{

		// Normalize the facing direction
		if (glm::length(m_FacingDirection) != 0.0f)
		{
			m_FacingDirection = glm::normalize(m_FacingDirection);
		}
		else
		{
			m_FacingDirection = glm::vec3(0, 0, 1); // Default facing direction if zero vector is provided
		}

		UpdateFinalPositions();
	}

	void Cuboid::UpdateFinalPositions()
	{
		// --- 1) Half-extents ---
		const float hx = 0.5f * std::abs(m_Width);
		const float hy = 0.5f * std::abs(m_Height);
		const float hz = 0.5f * std::abs(m_Depth);

		// --- 2) Local-space corners (centered at origin): +Z=Front, +Y=Top, +X=Right
		const glm::vec3 LBB(-hx, -hy, -hz);
		const glm::vec3 LBT(-hx, +hy, -hz);
		const glm::vec3 LFB(-hx, -hy, +hz);
		const glm::vec3 LFT(-hx, +hy, +hz);
		const glm::vec3 RBB(+hx, -hy, -hz);
		const glm::vec3 RBT(+hx, +hy, -hz);
		const glm::vec3 RFB(+hx, -hy, +hz);
		const glm::vec3 RFT(+hx, +hy, +hz);

		// --- 3) Build a stable orientation with an up reference to remove roll ambiguity ---
		glm::vec3 fwd = m_FacingDirection; // already normalized by ctor
		const glm::vec3 upRef(0.0f, 1.0f, 0.0f);

		// If facing is (almost) parallel to upRef, pick an alternate up to avoid degeneracy
		glm::vec3 tmpUp = upRef;
		if (std::abs(glm::dot(fwd, upRef)) > 0.999999f)
		{
			tmpUp = glm::vec3(1.0f, 0.0f, 0.0f); // world X as backup
		}

		glm::vec3 right = glm::normalize(glm::cross(tmpUp, fwd));
		glm::vec3 up = glm::normalize(glm::cross(fwd, right));

		// Column-major mat3: columns are basis vectors in world space
		glm::mat3 basis(1.0f);
		basis[0] = right; // X column
		basis[1] = up;	  // Y column
		basis[2] = fwd;	  // Z column

		glm::quat q_align = glm::quat_cast(basis);

		// Optional extra twist around the facing axis (world-space)
		glm::quat q_twist = glm::angleAxis(glm::radians(m_Roration_degrees), fwd);

		m_Orientation = glm::normalize(q_twist * q_align);

		// --- 4) Transform helper ---
		auto to_world = [&](const glm::vec3& p_local) -> glm::vec3 { return m_Center + (m_Orientation * p_local); };

		// --- 5) Write final positions (front/back defined in local +Z/-Z) ---
		m_FinalPosFrontBottomLeft = to_world(LFB);
		m_FinalPosFrontBottomRight = to_world(RFB);
		m_FinalPosFrontTopLeft = to_world(LFT);
		m_FinalPosFrontTopRight = to_world(RFT);

		m_FinalPosBackBottomLeft = to_world(LBB);
		m_FinalPosBackBottomRight = to_world(RBB);
		m_FinalPosBackTopLeft = to_world(LBT);
		m_FinalPosBackTopRight = to_world(RBT);
	}

	void Cuboid::Rotate(const glm::vec3& rotationPoint, const glm::vec3& rotationAngles_deg_local)
	{
		// Early-out if there's effectively no rotation
		if (rotationAngles_deg_local.x == 0.0f && rotationAngles_deg_local.y == 0.0f &&
			rotationAngles_deg_local.z == 0.0f)
		{
			return;
		}

		// 1) Build the local-space delta rotation from Euler angles (degrees -> radians).
		//    Order: local X then Y then Z (applied as qz*qy*qx when using quaternions).
		const glm::vec3 r = glm::radians(rotationAngles_deg_local);
		const glm::quat qx = glm::angleAxis(r.x, glm::vec3(1.0f, 0.0f, 0.0f));
		const glm::quat qy = glm::angleAxis(r.y, glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::quat qz = glm::angleAxis(r.z, glm::vec3(0.0f, 0.0f, 1.0f));
		const glm::quat deltaLocal = glm::normalize(qz * qy * qx);

		// 2) Save the current orientation to compute the equivalent world-space rotation.
		const glm::quat prevOrientation = m_Orientation;

		// 3) Update orientation by applying local rotation (post-multiply).
		//    (Orientation maps local->world; post-multiplying applies rotation in local frame.)
		m_Orientation = glm::normalize(m_Orientation * deltaLocal);

		// 4) Convert the local delta to a world-space rotation to rotate the position about rotationPoint.
		//    R_world = R_current * R_local * R_current^{-1}, but we need it relative to the *previous* frame:
		//    deltaWorld = prevOrientation * deltaLocal * conj(prevOrientation).
		const glm::quat deltaWorld = glm::normalize(prevOrientation * deltaLocal * glm::conjugate(prevOrientation));

		// 5) Rotate center around the given world-space pivot using the world-space delta.
		glm::vec3 rel = m_Center - rotationPoint;
		rel = deltaWorld * rel; // glm supports quat * vec3 for rotating vectors
		m_Center = rotationPoint + rel;

		// 6) Keep a convenient facing direction (local +Z transformed to world).
		m_FacingDirection = glm::normalize(m_Orientation * glm::vec3(0.0f, 0.0f, 1.0f));

		// 7) If you track an extra roll about facing direction (m_Roration_degrees),
		//    you can keep it separate; otherwise you can ignore or recompute it here.

		// 8) Recompute final face corner positions.
		UpdateFinalPositions();
	}

	Cuboid::FacePositions Cuboid::GetFrontFacePositions() const
	{
		// Normal  +Z
		return {
			m_FinalPosFrontBottomLeft,	// BottomLeft
			m_FinalPosFrontBottomRight, // BottomRight
			m_FinalPosFrontTopRight,	// TopRight
			m_FinalPosFrontTopLeft		// TopLeft
		};
	}

	Cuboid::FacePositions Cuboid::GetBackFacePositions() const
	{
		// Normal  -Z
		return {
			m_FinalPosBackBottomRight, // BottomLeft (as seen from back/outside)
			m_FinalPosBackBottomLeft,  // BottomRight
			m_FinalPosBackTopLeft,	   // TopRight
			m_FinalPosBackTopRight	   // TopLeft
		};
	}

	Cuboid::FacePositions Cuboid::GetLeftFacePositions() const
	{
		// Normal  -X
		return {
			m_FinalPosBackBottomLeft,  // BottomLeft
			m_FinalPosFrontBottomLeft, // BottomRight
			m_FinalPosFrontTopLeft,	   // TopRight
			m_FinalPosBackTopLeft	   // TopLeft
		};
	}

	Cuboid::FacePositions Cuboid::GetRightFacePositions() const
	{
		// Normal  +X
		return {
			m_FinalPosFrontBottomRight, // BottomLeft
			m_FinalPosBackBottomRight,	// BottomRight
			m_FinalPosBackTopRight,		// TopRight
			m_FinalPosFrontTopRight		// TopLeft
		};
	}

	Cuboid::FacePositions Cuboid::GetTopFacePositions() const
	{
		// Normal  +Y
		return {
			m_FinalPosFrontTopLeft,	 // BottomLeft
			m_FinalPosFrontTopRight, // BottomRight
			m_FinalPosBackTopRight,	 // TopRight
			m_FinalPosBackTopLeft	 // TopLeft
		};
	}

	Cuboid::FacePositions Cuboid::GetBottomFacePositions() const
	{
		// Normal  -Y
		return {
			m_FinalPosBackBottomLeft,	// BottomLeft
			m_FinalPosBackBottomRight,	// BottomRight
			m_FinalPosFrontBottomRight, // TopRight
			m_FinalPosFrontBottomLeft	// TopLeft
		};
	}
} // namespace onion::voxel
