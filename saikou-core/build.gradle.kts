plugins {
    kotlin("multiplatform") version "1.9.22"
    kotlin("plugin.serialization") version "1.9.22"
}

repositories {
    mavenCentral()
}

kotlin {
    linuxX64 {
        binaries {
            sharedLib {
                baseName = "saikou_core"
            }
        }
    }
    
    sourceSets {
        val commonMain by getting {
            dependencies {
                implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.8.0")
                implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.6.3")
                implementation("io.ktor:ktor-client-core:2.3.11")
                implementation("io.ktor:ktor-client-content-negotiation:2.3.11")
                implementation("io.ktor:ktor-serialization-kotlinx-json:2.3.11")
                implementation("com.fleeksoft.ksoup:ksoup:0.1.2") // If I need it, let's use it
            }
        }
        val linuxX64Main by getting {
            dependencies {
                implementation("io.ktor:ktor-client-curl:2.3.11")
            }
        }
    }
}
