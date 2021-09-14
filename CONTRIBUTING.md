# Contributing

## Code of Conduct
This project and everyone participating in it is governed by the Code of Conduct. By participating, you are expected to uphold this code. Please report unacceptable behavior to the VARID Consortium.

## Bugs & Enhancements
- Use Github built in issue tracking
- https://github.com/VARIDConsortium/ue4-varid-plugin/issues

## Pull Requests
- Pull requests are required. 
- Commit message format: should have at least type e.g. add, fix and some context e.g. rendering, profile, FX

## General Coding Styles
- https://github.com/Allar/ue4-style-guide
- https://docs.unrealengine.com/4.26/en-US/ProductionPipelines/DevelopmentSetup/CodingStandard/

## Specific Coding Styles
- VARID Prefix
  - prefix main class/objects with VARID. 
  - Its a short, unique and makes it easy to identify/namespace all VARID code. 
- Shaders
  - Within C++, prefer calling the parameter object 'PassParameters'. This is inline with Unreal Engine code. 
  - Place the parameters and call to AddPass in a local curly bracket {} scope. This helps prevent variable leakage. 
- Alphabetical ordering
  - Where applicable always work with VARID concepts in a consistent alphabetical order. e.g. for FX
    - The json profile lists the FX as sub nodes in alphabetical order. 
    - The render method builds up FX VF Maps in alphabetical order. 
- Function parameters
  - all input parameters prefixed with In
  - all output parameter prefixed with Out