plugins {
    alias(libs.plugins.kotlin.jvm)
    alias(libs.plugins.kotlin.serialization)
    application
}

kotlin {
    jvmToolchain(21)
}

dependencies {
    implementation(libs.coroutines.core)
    implementation(libs.serialization.json)
    implementation(libs.okhttp)
    implementation(libs.jsoup)

    testImplementation(libs.kotlin.test)
}

application {
    mainClass.set("ani.saikou.MainKt")
    applicationName = "saikou-core"
}

tasks.test {
    useJUnitPlatform()
}

// `vendor/android-src` holds the unported Android sources. It is deliberately outside
// every compiled source set: files move into `src/main/kotlin` as they are ported.
