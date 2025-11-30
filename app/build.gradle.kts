import java.net.URI
import java.io.FileOutputStream

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

android {
    signingConfigs {
        getByName("debug"){
            storeFile = rootProject.file("debug.keystore")
        }
        create("release") {
            storeFile = rootProject.file("release.keystore")
            storePassword = System.getenv("KEYSTORE_PASSWORD")
            keyAlias = System.getenv("KEY_ALIAS")
            keyPassword = System.getenv("KEY_PASSWORD")
        }
    }
    namespace = "com.gokrack.beatriceapp"
    compileSdk {
        version = release(36)
    }

    packagingOptions {
        jniLibs {
            useLegacyPackaging = true
        }
    }

    defaultConfig {
        applicationId = "com.gokrack.beatriceapp"
        minSdk = 30
        targetSdk = 36
        versionCode = 1
        versionName = "0.0.3"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        ndk {
            abiFilters += listOf("arm64-v8a")//, "x86_64")
        }
        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
                arguments += "-DANDROID_STL=c++_shared"
                abiFilters += listOf("arm64-v8a")//, "x86_64")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs.getByName("release")
        }
        getByName("debug") {
            signingConfig = signingConfigs.getByName("debug")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_18
        targetCompatibility = JavaVersion.VERSION_18
    }
    kotlinOptions {
        jvmTarget = "18"
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    buildFeatures {
        viewBinding = true
        prefab = true
    }
    ndkVersion = "29.0.14206865"
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.constraintlayout)
    implementation("com.google.oboe:oboe:1.10.0")
    implementation(project(":audio-device"))
    implementation(libs.androidx.documentfile)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
}

val downloadBeatriceLib_ARM by tasks.registering {
    val targetDir = file("../lib/beatrice-api/arm64-v8a/")
    val targetFile = File(targetDir, "libbeatrice.a")
    val fileUri = URI("https://huggingface.co/GokRack/beatrice-api-for-android/resolve/rc.0/arm64-v8a/libbeatrice.a")

    outputs.file(targetFile)

    doLast {
        targetDir.mkdirs()
        fileUri.toURL().openStream().use { input ->
            FileOutputStream(targetFile).use { output ->
                input.copyTo(output)
            }
        }
        println("Downloaded to: ${targetFile.absolutePath}")
    }
    onlyIf{
        !targetFile.exists()
    }
}

val downloadBeatriceLib_x64 by tasks.registering {
    val targetDir = file("../lib/beatrice-api/x86_64/")
    val targetFile = File(targetDir, "libbeatrice.a")
    val fileUri = URI("https://huggingface.co/GokRack/beatrice-api-for-android/resolve/rc.0/x86_64/libbeatrice.a")

    outputs.file(targetFile)

    doLast {
        targetDir.mkdirs()
        fileUri.toURL().openStream().use { input ->
            FileOutputStream(targetFile).use { output ->
                input.copyTo(output)
            }
        }
        println("Downloaded to: ${targetFile.absolutePath}")
    }
    onlyIf{
        !targetFile.exists()
    }
}

val downloadDefaultModelForAssets by tasks.registering {
    val assetList = listOf(
        URI("https://huggingface.co/fierce-cats/beatrice-2.0.0-alpha/resolve/rc.0/rc.0/beatrice_paraphernalia_jvs_156_03000000/beatrice_paraphernalia_jvs.toml"),
        URI("https://huggingface.co/fierce-cats/beatrice-2.0.0-alpha/resolve/rc.0/rc.0/beatrice_paraphernalia_jvs_156_03000000/embedding_setter.bin"),
        URI("https://huggingface.co/fierce-cats/beatrice-2.0.0-alpha/resolve/rc.0/rc.0/beatrice_paraphernalia_jvs_156_03000000/noimage.png"),
        URI("https://huggingface.co/fierce-cats/beatrice-2.0.0-alpha/resolve/rc.0/rc.0/beatrice_paraphernalia_jvs_156_03000000/phone_extractor.bin"),
        URI("https://huggingface.co/fierce-cats/beatrice-2.0.0-alpha/resolve/rc.0/rc.0/beatrice_paraphernalia_jvs_156_03000000/pitch_estimator.bin"),
        URI("https://huggingface.co/fierce-cats/beatrice-2.0.0-alpha/resolve/rc.0/rc.0/beatrice_paraphernalia_jvs_156_03000000/speaker_embeddings.bin"),
        URI("https://huggingface.co/fierce-cats/beatrice-2.0.0-alpha/resolve/rc.0/rc.0/beatrice_paraphernalia_jvs_156_03000000/waveform_generator.bin")
    )
    val outputDir = file("src/main/assets/beatrice_paraphernalia_jvs/")

    // 出力ファイル群を Gradle に認識させる
    outputs.files(assetList.map { uri ->
        val fileName = uri.path.substringAfterLast("/")
        File(outputDir, fileName)
    })

    doLast {
        outputDir.mkdirs()
        assetList.forEach { uri ->
            val fileName = uri.path.substringAfterLast("/")
            val outputFile = File(outputDir, fileName)

            if (!outputFile.exists()) {
                println("Downloading: $fileName")
                uri.toURL().openStream().use { input ->
                    FileOutputStream(outputFile).use { output ->
                        input.copyTo(output)
                    }
                }
                println("Saved to: ${outputFile.absolutePath}")
            } else {
                println("Already exists: $fileName — skipping")
            }
        }
    }

    // すべてのファイルが存在しないときだけ実行
    onlyIf {
        assetList.any { uri ->
            val fileName = uri.path.substringAfterLast("/")
            val outputFile = File(outputDir, fileName)
            !outputFile.exists()
        }
    }
}

tasks.named("preBuild") {
    dependsOn(downloadBeatriceLib_ARM)
    //dependsOn(downloadBeatriceLib_x64)
    dependsOn(downloadDefaultModelForAssets)
}