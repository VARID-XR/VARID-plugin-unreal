# Contributing


pull requests
commit message format: should have at least type e.g. add, fix and some context e.g. rendering, profile, blur
code style guide - https://github.com/Allar/ue4-style-guide
reporting bugs
suggesting enhancements

Where applicable always work with FX in alphabetical order. e.g. the json profile lists the FX as sub nodes in alphabetical order

prefix main class/objects with VARID. Its a short, unique and makes it easy to identify/namespace all VARID code. 

for all functions in hlsl and c++ code
all input parameters prefixed with In
all output parameter prefixed with Out

shaders
declaring shader parameters. in line with Unreal prefer calling the parameter object 'PassParameters'
Place the parameters and call to AddPass in a local {} scope. 


https://docs.unrealengine.com/4.26/en-US/ProductionPipelines/DevelopmentSetup/CodingStandard/#exampleformatting

Code of Conduct
This project and everyone participating in it is governed by the Code of Conduct. By participating, you are expected to uphold this code. Please report unacceptable behavior to <<CONTACT DETAILS>>