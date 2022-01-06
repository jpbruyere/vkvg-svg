FILE(GLOB sources src/*.c)

FIND_PACKAGE(Vulkan REQUIRED)
FIND_PACKAGE(GLFW3  REQUIRED)

FILE(GLOB sources "*.c")

ADD_EXECUTABLE(svg-viewer ${sources})

TARGET_INCLUDE_DIRECTORIES(svg-viewer PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/tests"
	"${GLFW3_INCLUDE_DIR}"
	"${Vulkan_INCLUDE_DIRS}"

)
TARGET_LINK_LIBRARIES(svg-viewer
	vkvg-svg
	"${GLFW3_LIBRARY}"
	"${Vulkan_LIBRARIES}"
	"$<$<PLATFORM_ID:Linux>:-lm>"
)

