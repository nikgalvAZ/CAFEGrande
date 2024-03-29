/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/exceptions/CameraException.h"

/* Local */
#include "backend/Camera.h"

pm::CameraException::CameraException(const std::string& what, Camera* camera)
    : pm::Exception(what),
    m_camera(camera),
    m_what()
{
}

const char* pm::CameraException::what() const noexcept
{
    // Message has to be expanded later as error message could be set upon
    // thrown exception, e.g. for fake cameras
    m_what = std::string(Base::what())
        + " [" + m_camera->GetErrorMessage() + "]";
    return m_what.c_str();
}

pm::Camera* pm::CameraException::GetCamera() const
{
    return m_camera;
}
