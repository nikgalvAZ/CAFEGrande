/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_OPTION_CONTROLLER_H
#define PM_OPTION_CONTROLLER_H

/* Local */
#include "backend/Option.h"

/* System */
#include <vector>
#include <string>

namespace pm {

class OptionController
{
public:
    OptionController();

public:
    // Add new unique option
    bool AddOption(const Option& option);

    // Process parameters and run the command line options handlers for all
    // registered options.
    // Options are processed in the same order they were added.
    bool ProcessOptions(int argc, char* argv[]);

    // Same as ProcessOptions above but limited to given options.
    // Options are processed in the same order they were added.
    bool ProcessOptions(int argc, char* argv[],
            const std::vector<Option>& options, bool ignoreUnknown = false);

    // Get string with CLI options usage
    std::string GetOptionsDescription(const std::vector<Option>& options,
            bool includeHeader = true) const;

    // Registered options
    const std::vector<Option>& GetOptions() const
    { return m_options; }

    // Registered and successfully handled options
    const std::vector<Option>& GetAllProcessedOptions() const
    { return m_optionsPassed; }

    // Registered options that failed while running assigned handler
    const std::vector<Option>& GetFailedProcessedOptions() const
    { return m_optionsPassedFailed; }

private:
    // Verify that there are no duplicate options registered
    bool VerifyOptions(std::string& conflictingName) const;

private:
    std::vector<Option> m_options{};
    std::vector<Option> m_optionsPassed{};
    std::vector<Option> m_optionsPassedFailed{};
};

} // namespace pm

#endif /* PM_OPTION_CONTROLLER_H */
