message(STATUS "Building test programs")

if (USE_PYTHON_BINDINGS)
  add_test(NAME "tester-python-element-type-bindings"
           COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python/elementtype_bindings.py)
  set_tests_properties("tester-python-element-type-bindings" PROPERTIES
                       ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/python")

  add_test(NAME "tester-python-laser-bindings"
           COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python/laser_bindings.py)
  set_tests_properties("tester-python-laser-bindings" PROPERTIES
                       ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/python")
endif()

add_executable(BDSFieldTester BDSFieldTester.cc)
set_target_properties(BDSFieldTester PROPERTIES OUTPUT_NAME "BDSFieldTest" VERSION ${BDSIM_VERSION})
target_link_libraries(BDSFieldTester ${BDSIM_LIB_NAME} ${GMAD_LIB_NAME})
configure_file(purefieldplotter.py purefieldplotter.py COPYONLY)
configure_file(converttobdsim.py converttobdsim.py COPYONLY)
add_test(NAME "tester-fields" COMMAND BDSFieldTester)

add_executable(BDSInterpolatorTester BDSInterpolatorTester.cc)
set_target_properties(BDSInterpolatorTester PROPERTIES OUTPUT_NAME "BDSInterpolatorTester" VERSION ${BDSIM_VERSION})
target_link_libraries(BDSInterpolatorTester ${BDSIM_LIB_NAME} ${GMAD_LIB_NAME})
add_test(NAME "tester-interpolator" COMMAND BDSInterpolatorTester)

if (USE_GDML)
  add_executable(BDSGDMLSchemaTester BDSGDMLSchemaTester.cc)
  target_link_libraries(BDSGDMLSchemaTester ${BDSIM_LIB_NAME})
  add_test(NAME "tester-gdml-schema"
           COMMAND BDSGDMLSchemaTester "${CMAKE_SOURCE_DIR}/src-external")
  set_tests_properties("tester-gdml-schema" PROPERTIES
                       ENVIRONMENT "BDSIM_DATA_DIR=${CMAKE_SOURCE_DIR}/src-external")
endif()

add_executable(BDSLinkTester BDSLinkTester.cc)
set_target_properties(BDSLinkTester PROPERTIES OUTPUT_NAME "BDSLinkTester" VERSION ${BDSIM_VERSION})
target_link_libraries(BDSLinkTester ${BDSIM_LIB_NAME} gmad)
#add_test(NAME "tester-link" COMMAND BDSLinkTester)

add_executable(BDSLinkSamplerTester BDSLinkSamplerTester.cc)
target_link_libraries(BDSLinkSamplerTester ${BDSIM_LIB_NAME} gmad)
add_test(NAME "tester-link-sampler-parser-first"
         COMMAND BDSLinkSamplerTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_sampler.gmad parser probe0)
add_test(NAME "tester-link-sampler-parser-last"
         COMMAND BDSLinkSamplerTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_sampler.gmad parser probe1)
add_test(NAME "tester-link-sampler-legacy"
         COMMAND BDSLinkSamplerTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_sampler_empty.gmad legacy probe)

add_executable(BDSLinkComponentIndexTester BDSLinkComponentIndexTester.cc)
target_link_libraries(BDSLinkComponentIndexTester ${BDSIM_LIB_NAME} gmad)
add_test(NAME "tester-link-component-indexing"
         COMMAND BDSLinkComponentIndexTester
                 ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_component_indexing.gmad)

add_executable(BDSLinkFieldTester BDSLinkFieldTester.cc)
target_link_libraries(BDSLinkFieldTester ${BDSIM_LIB_NAME} gmad)
add_test(NAME "tester-link-field-construction"
         COMMAND BDSLinkFieldTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_field.gmad probe)
add_test(NAME "tester-link-field-map"
         COMMAND BDSLinkFieldTester
                 ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_field_map.gmad
                 probe_map 0.00962872616947 -0.00148509279825)

add_executable(BDSLinkStraightElementTester BDSLinkStraightElementTester.cc)
target_link_libraries(BDSLinkStraightElementTester ${BDSIM_LIB_NAME} gmad)
add_test(NAME "tester-link-straight-drift"
         COMMAND BDSLinkStraightElementTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_straight_elements.gmad d1 transport)
add_test(NAME "tester-link-straight-gap"
         COMMAND BDSLinkStraightElementTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_straight_elements.gmad g1 transport)
foreach(element_name q1 s1 o1 dc1 mul1)
  add_test(NAME "tester-link-straight-${element_name}"
           COMMAND BDSLinkStraightElementTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_straight_elements.gmad ${element_name} field)
endforeach()

add_executable(BDSLinkInputOffsetTester BDSLinkInputOffsetTester.cc)
target_link_libraries(BDSLinkInputOffsetTester ${BDSIM_LIB_NAME} gmad)
add_test(NAME "tester-link-input-offset"
         COMMAND BDSLinkInputOffsetTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_input_offset.gmad)

add_executable(BDSLinkCurvilinearTester BDSLinkCurvilinearTester.cc)
target_link_libraries(BDSLinkCurvilinearTester ${BDSIM_LIB_NAME} gmad)
add_test(NAME "tester-link-curvilinear-sbend"
         COMMAND BDSLinkCurvilinearTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_sbend.gmad)
add_test(NAME "tester-link-curvilinear-rbend"
         COMMAND BDSLinkCurvilinearTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_rbend.gmad)

add_executable(BDSLinkComponentLineTester BDSLinkComponentLineTester.cc)
target_link_libraries(BDSLinkComponentLineTester ${BDSIM_LIB_NAME} gmad)
add_test(NAME "tester-link-component-line-rf"
         COMMAND BDSLinkComponentLineTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_component_lines.gmad rf1 energy)
add_test(NAME "tester-link-component-line-rfx"
         COMMAND BDSLinkComponentLineTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_component_lines.gmad rfx1 field-x)
add_test(NAME "tester-link-component-line-rfy"
         COMMAND BDSLinkComponentLineTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_component_lines.gmad rfy1 field-y)
foreach(element_name sol1 hk1)
  add_test(NAME "tester-link-component-line-${element_name}"
           COMMAND BDSLinkComponentLineTester ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_component_lines.gmad ${element_name} field)
endforeach()

if (USE_GDML)
  add_executable(BDSLinkExternalGeometryTester BDSLinkExternalGeometryTester.cc)
  target_link_libraries(BDSLinkExternalGeometryTester ${BDSIM_LIB_NAME} gmad)
  add_test(NAME "tester-link-external-geometry"
           COMMAND BDSLinkExternalGeometryTester
                   ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_external_geometry.gmad)
  set_tests_properties("tester-link-external-geometry" PROPERTIES
    ENVIRONMENT "BDSIM_DATA_DIR=${CMAKE_SOURCE_DIR}/src-external")
endif()

add_executable(BDSLinkNativeElementTester BDSLinkNativeElementTester.cc)
target_link_libraries(BDSLinkNativeElementTester ${BDSIM_LIB_NAME} gmad)
function(add_link_native_element_test element_name length_m outcome)
  add_test(NAME "tester-link-native-${element_name}"
           COMMAND BDSLinkNativeElementTester
                   ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_remaining_native_elements.gmad
                   ${element_name} ${length_m} ${outcome})
  set_tests_properties("tester-link-native-${element_name}" PROPERTIES
    FAIL_REGULAR_EXPRESSION "Overlap with mother volume")
endfunction()
add_link_native_element_test(target1      0.05 transport)
add_link_native_element_test(dump1        0.2  absorbed)
add_link_native_element_test(shield1      0.2  transport)
add_link_native_element_test(degrader1    0.4  transport)
add_link_native_element_test(gascap1      0.5  transport)
add_link_native_element_test(gasjet1      0.15 transport)
add_link_native_element_test(screen1      0.1  transport)
add_link_native_element_test(wire1        0.1  transport)
add_link_native_element_test(jcoltip1     0.2  transport)
add_link_native_element_test(undulator1   0.4  transport)
add_link_native_element_test(gabor1       0.52 transport)
add_link_native_element_test(muonspoiler1 1.0  transport)

add_executable(BDSLinkLaserElementTester BDSLinkLaserElementTester.cc)
target_link_libraries(BDSLinkLaserElementTester ${BDSIM_LIB_NAME} gmad)
add_test(NAME "tester-link-laser-legacy"
         COMMAND BDSLinkLaserElementTester
                 ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_laser_elements.gmad
                 legacyLaser legacy-compton)
add_test(NAME "tester-link-laserwire"
         COMMAND BDSLinkLaserElementTester
                 ${CMAKE_CURRENT_SOURCE_DIR}/bdslink/link_laser_elements.gmad
                 modernLaser transport)

add_executable(BDSSixTrackTester BDSSixTrackTester.cc)
set_target_properties(BDSSixTrackTester PROPERTIES OUTPUT_NAME "BDSSixTrackTester" VERSION ${BDSIM_VERSION})
target_link_libraries(BDSSixTrackTester ${BDSIM_LIB_NAME} gmad)
add_test(NAME "tester-sixtrack" COMMAND BDSSixTrackTester)
configure_file(somecollimators.dat somecollimators.dat COPYONLY)
configure_file(lhccrystals.gmad lhccrystals.gmad COPYONLY)
# prepare non-tested visualiser copy
add_executable(BDSSixTrackTesterVis BDSSixTrackTester.cc)
set_target_properties(BDSSixTrackTesterVis PROPERTIES OUTPUT_NAME "BDSSixTrackTesterVis" VERSION ${BDSIM_VERSION})
target_compile_definitions(BDSSixTrackTesterVis PUBLIC -DVISLINK)
target_link_libraries(BDSSixTrackTesterVis ${BDSIM_LIB_NAME} gmad)

add_executable(BDSTrajectoryTester BDSTrajectoryTester.cc)
set_target_properties(BDSTrajectoryTester PROPERTIES OUTPUT_NAME "BDSTrajectoryTest" VERSION ${BDSIM_VERSION})
target_link_libraries(BDSTrajectoryTester rebdsim bdsimRootEvent bdsim)
add_test(NAME "tester-trajectories" COMMAND BDSTrajectoryTester "../examples/features/data/trajectory-sample.root")

add_executable(BDSModelTreeTester BDSModelTreeTester.cc)
set_target_properties(BDSModelTreeTester PROPERTIES OUTPUT_NAME "BDSModelTreeTest" VERSION ${BDSIM_VERSION})
target_link_libraries(BDSModelTreeTester rebdsim bdsimRootEvent bdsim)
add_test(NAME "tester-model-tree" COMMAND BDSModelTreeTester "../examples/features/data/sample1.root")

add_executable(TH1SetTest TH1SetTest.cc)
target_link_libraries(TH1SetTest ${BDSIM_LIB_NAME} ${ROOT_LIBRARIES} rebdsim)

add_subdirectory(TrackingTestFiles)
