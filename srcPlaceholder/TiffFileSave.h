/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef _TIFF_FILE_SAVE_H
#define _TIFF_FILE_SAVE_H

/* Local */
#include "backend/FileSave.h"

// Forward declaration for md_frame that satisfies compiler (taken from pvcam.h)
struct md_frame;
typedef struct md_frame md_frame;

// Forward declaration for TIFF that satisfies compiler (taken from tiffio.h)
struct tiff;
typedef struct tiff TIFF;

// Forward declaration for color context (taken from pvcam_helper_color.h)
struct ph_color_context;

namespace pm {

class Bitmap;
class FrameProcessor;

class TiffFileSave final : public FileSave
{
public:
    // A structure either created internally or passed via constructor.
    // Used in WriteFrame methods to reconstruct whole frame.
    // Internally created instance stores raw unaltered pixel data, i.e.
    // no debayering nor white-balancing for color images, background is filled
    // with black pixels.
    struct Helper
    {
        // The processor doing all hard work
        FrameProcessor* frameProc{ nullptr };
        // Storage for final recomposed frame that will be stored in file
        Bitmap* fullBmp{ nullptr };
        // Color context for debayering and white-balancing or nullptr for mono
        ph_color_context* colorCtx{ nullptr };
        // Use any value valid for current bit depth, use zero value for
        // black-filling, or set to negative value less than -0.5 to
        // automatically fill each frame with its mean value.
        double fillValue{ 0.0 };
    };


public:
    TiffFileSave(const std::string& fileName, PrdHeader& header,
            bool useBigTiff = false);
    TiffFileSave(const std::string& fileName, PrdHeader& header,
            Helper* helper, bool useBigTiff = false);
    virtual ~TiffFileSave();

    TiffFileSave() = delete;
    TiffFileSave(const TiffFileSave&) = delete;
    TiffFileSave(TiffFileSave&&) = delete;
    TiffFileSave& operator=(const TiffFileSave&) = delete;
    TiffFileSave& operator=(TiffFileSave&&) = delete;

public: // From File
    virtual bool Open() override;
    virtual bool IsOpen() const override;
    virtual void Close() override;

public: // From FileSave
    virtual bool WriteFrame(const void* metaData, const void* extDynMetaData,
            const void* rawData) override;
    virtual bool WriteFrame(std::shared_ptr<Frame> frame) override;

private:
    bool DoWriteFrame(std::shared_ptr<Frame> frame);
    bool DoWriteTiff(const Bitmap* bmp, const std::string& imageDesc);

private:
    TIFF* m_file{ nullptr };
    Helper* m_helper{ nullptr };
    const bool m_helperOwned;
    const bool m_isBigTiff;
};

} // namespace pm

#endif /* _TIFF_FILE_SAVE_H */
