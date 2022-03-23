add_executable(82m2wav 82m2wav.cpp)
add_executable(bin2txt bin2txt.cpp)
add_executable(extractor extractor.cpp)
add_executable(icn2img icn2img.cpp)
add_executable(til2img til2img.cpp)
add_executable(xmi2mid xmi2mid_cli.cpp)

target_compile_definitions(icn2img PRIVATE
	$<$<BOOL:${ENABLE_IMAGE}>:WITH_IMAGE>
	)
target_compile_definitions(til2img PRIVATE
	$<$<BOOL:${ENABLE_IMAGE}>:WITH_IMAGE>
	)

target_link_libraries(82m2wav
	engine
	)
target_link_libraries(bin2txt
	engine
	)
target_link_libraries(extractor
	engine
	)
target_link_libraries(icn2img
	engine
	)
target_link_libraries(til2img
	engine
	)
target_link_libraries(xmi2mid
	engine
	)
