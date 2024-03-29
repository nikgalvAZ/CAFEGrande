/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_CAMERA_EXCEPTION_H
#define PM_CAMERA_EXCEPTION_H

/* Local */
#include "backend/exceptions/Exception.h"

namespace pm {

class Camera;

class CameraException : public pm::Exception
{
public:
    using Base = Exception;

public:
    CameraException(const std::string& what, Camera* camera);

public: // Base
    virtual const char* what() const noexcept override;

public:
    Camera* GetCamera() const;

protected:
    Camera* m_camera;
    mutable std::string m_what; // Mutable, is set in what method
};

} // namespace pm

#endif /* PM_CAMERA_EXCEPTION_H */
