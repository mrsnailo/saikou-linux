plugins {
    alias(libs.plugins.kotlin.jvm)
    alias(libs.plugins.kotlin.serialization)
    application
}

kotlin {
    jvmToolchain(21)
}

/**
 * The AniList API client this build signs in with.
 *
 * AniList only accepts the authorization-code grant — it rejects `response_type=token`
 * with `unsupported_grant_type`, and its token endpoint answers `invalid_client` without
 * a secret, so PKCE is not an option either. One-click sign-in therefore requires the app
 * to hold a secret, and holding it means keeping it out of the repository: it is injected
 * here from the environment (or a Gradle property) and never committed.
 *
 * A build without it still works; sign-in then asks the user for a client of their own.
 */
val anilistClientId = providers.environmentVariable("SAIKOU_ANILIST_CLIENT_ID")
    .orElse(providers.gradleProperty("saikou.anilist.clientId"))
    .orElse("")
val anilistClientSecret = providers.environmentVariable("SAIKOU_ANILIST_CLIENT_SECRET")
    .orElse(providers.gradleProperty("saikou.anilist.clientSecret"))
    .orElse("")

val generateBundledClient by tasks.registering {
    val outputDir = layout.buildDirectory.dir("generated/source/anilist")
    val id = anilistClientId
    val secret = anilistClientSecret

    inputs.property("clientId", id)
    inputs.property("clientSecret", secret)
    outputs.dir(outputDir)

    doLast {
        val file = outputDir.get().file("ani/saikou/anilist/BundledClient.kt").asFile
        file.parentFile.mkdirs()
        file.writeText(
            """
            package ani.saikou.anilist

            /** Generated at build time. See the block that writes it in build.gradle.kts. */
            internal object BundledClient {
                const val ID = "${id.get().kotlinEscaped()}"
                const val SECRET = "${secret.get().kotlinEscaped()}"
            }
            """.trimIndent() + "\n"
        )
    }
}

fun String.kotlinEscaped(): String = replace("\\", "\\\\").replace("\"", "\\\"").replace("$", "\${'$'}")

kotlin.sourceSets.named("main") {
    kotlin.srcDir(generateBundledClient)
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
