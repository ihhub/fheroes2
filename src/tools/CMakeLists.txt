add_executable(82m2wav 82m2wav.cpp)
target_link_libraries(82m2wav
	engine
	)

add_executable(bin2txt bin2txt.cpp)

add_executable(extractor extractor.cpp)
target_link_libraries(extractor
	engine
	)

add_executable(icn2img icn2img.cpp)
target_link_libraries(icn2img
	engine
	)

add_executable(til2img til2img.cpp)
target_link_libraries(til2img
	engine
	)

add_executable(xmi2mid xmi2mid_cli.cpp)
target_link_libraries(xmi2mid
	engine
	)
