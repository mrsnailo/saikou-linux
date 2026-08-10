plugins {
    // Resolves the JDK the build asks for (21, see saikou-core/build.gradle.kts) instead
    // of failing when the machine only has a newer one. Arch ships whatever JDK is current
    // — 26 at the time of writing — and a packager should not have to install a second one
    // by hand for the build to work.
    id("org.gradle.toolchains.foojay-resolver-convention") version "1.0.0"
}

rootProject.name = "saikou-linux"

include(":saikou-core")

dependencyResolutionManagement {
    repositories {
        mavenCentral()
    }
}
