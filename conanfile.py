from conan import ConanFile

class Task(ConanFile):
    name = "Task"
    version = "0.1"
    settings = "os", "compiler", "arch", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("libpqxx/7.9.0")