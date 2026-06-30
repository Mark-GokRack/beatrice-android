package com.gokrack.beatriceapp

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.DocumentsContract
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.Spinner
import android.widget.ImageView
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.ActivityResultLauncher
import androidx.core.content.ContextCompat
import androidx.activity.result.contract.ActivityResultContracts
import androidx.documentfile.provider.DocumentFile
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import android.graphics.BitmapFactory
import java.io.File
import java.io.FileOutputStream

class VoiceFragment : Fragment() {

    private lateinit var viewModel: EngineStateViewModel
    private lateinit var modelPickerLauncher: ActivityResultLauncher<Intent>
    private lateinit var voiceSpinner: Spinner
    private lateinit var modelNameText: TextView
    private lateinit var modelDescriptionText: TextView
    private lateinit var voiceDescriptionText: TextView
    private lateinit var voicePortraitDescriptionText: TextView
    private lateinit var voicePortraitImage: ImageView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        viewModel = ViewModelProvider(requireActivity())[EngineStateViewModel::class.java]

        modelPickerLauncher = registerForActivityResult(
            ActivityResultContracts.StartActivityForResult()
        ) { result ->
            if (result.resultCode == Activity.RESULT_OK) {
                val treeUri = result.data?.data ?: return@registerForActivityResult
                requireActivity().contentResolver.takePersistableUriPermission(
                    treeUri,
                    Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                )
                val rootDir = DocumentFile.fromTreeUri(requireContext(), treeUri)
                val tomlFiles = rootDir?.listFiles()?.filter { file ->
                    file.isFile && file.name?.endsWith(".toml", ignoreCase = true) == true
                } ?: emptyList()

                if (tomlFiles.isNotEmpty()) {
                    copyModelFilesToExtDir(tomlFiles.first().uri)
                } else {
                    Toast.makeText(
                        requireContext(),
                        "TOMLファイルを含むフォルダを選択してください",
                        Toast.LENGTH_SHORT
                    ).show()
                }
            }
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = inflater.inflate(R.layout.fragment_voice, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        modelNameText = view.findViewById(R.id.ModelName)
        voiceSpinner = view.findViewById(R.id.voice_select_spinner)
        modelDescriptionText = view.findViewById(R.id.model_description)
        voiceDescriptionText = view.findViewById(R.id.voice_description)
        voicePortraitDescriptionText = view.findViewById(R.id.voice_portrait_description)
        voicePortraitImage = view.findViewById(R.id.voice_portrait_image)

        view.findViewById<Button>(R.id.button_model_select).setOnClickListener {
            val intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE).apply {
                flags = Intent.FLAG_GRANT_READ_URI_PERMISSION or
                        Intent.FLAG_GRANT_WRITE_URI_PERMISSION or
                        Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
            }
            modelPickerLauncher.launch(intent)
        }

        voiceSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(
                parent: AdapterView<*>, view: View?, position: Int, id: Long
            ) {
                beatriceEngine.setVoiceID(position)
                updateVoiceDetails(position)
            }
            override fun onNothingSelected(parent: AdapterView<*>) {}
        }

        viewModel.modelName.observe(viewLifecycleOwner) { name ->
            modelNameText.text = if (name.isNotEmpty()) name else getString(R.string.model_name)
        }

        viewModel.voiceNames.observe(viewLifecycleOwner) { names ->
            val adapter = ArrayAdapter(
                requireContext(),
                android.R.layout.simple_spinner_item,
                names
            )
            adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
            voiceSpinner.adapter = adapter
        }

        updateModelInfo()
    }

    // ---- Model file management ----

    private fun copyModelFilesToExtDir(tomlUri: Uri) {
        val tomlFile = DocumentFile.fromSingleUri(requireContext(), tomlUri) ?: return
        val docId = DocumentsContract.getDocumentId(tomlUri)
        val parentDocId = docId.substringBeforeLast("/")
        val parentUri = DocumentsContract.buildTreeDocumentUri(
            tomlUri.authority ?: return, parentDocId
        )
        val parentDir = requireNotNull(DocumentFile.fromTreeUri(requireContext(), parentUri))

        val destRoot = requireActivity().getExternalFilesDir(null) ?: requireActivity().filesDir
        deleteDirectoryContents(destRoot)
        copyFilteredFilesRecursively(parentDir, destRoot, "")
        beatriceEngine.readModel(
            tomlFile.name?.let { destRoot.resolve(it).absolutePath } ?: return
        )
        updateModelInfo()
    }

    private fun updateModelInfo() {
        val name = beatriceEngine.getModelName()
        viewModel.modelName.postValue(name)

        val description = beatriceEngine.getModelDescription()
        modelDescriptionText.text = description

        val voiceNameList = ArrayList<String>()
        var voiceCount = 0
        for (i in 0 until 256) {
            val voiceName = beatriceEngine.getVoiceName(i)
            if (voiceName.isNotEmpty()){ 
                voiceNameList.add(voiceName)
                voiceCount++
            }else{ break }
        }
        if( voiceCount > 1 ){
            voiceNameList.add( "VoiceMorphingMode");
        }
        viewModel.voiceNames.postValue(voiceNameList)
        beatriceEngine.setVoiceID(0)
        updateVoiceDetails(0)
    }

    private fun updateVoiceDetails(position: Int) {
        val description = beatriceEngine.getVoiceDescription(position)
        val relativePortraitPath = beatriceEngine.getVoicePortraitPath(position)
        val portraitDesc = beatriceEngine.getVoicePortraitDescription(position)

        voiceDescriptionText.text = description
        voicePortraitDescriptionText.text = portraitDesc

        if (relativePortraitPath.isNotEmpty()) {
            try {
                val modelRoot = requireActivity().getExternalFilesDir(null) ?: requireActivity().filesDir
                val absolutePortraitPath = File(modelRoot, relativePortraitPath).absolutePath
                val bitmap = BitmapFactory.decodeFile(absolutePortraitPath)
                if (bitmap != null) {
                    voicePortraitImage.setImageBitmap(bitmap)
                    voicePortraitImage.visibility = View.VISIBLE
                } else {
                    voicePortraitImage.visibility = View.GONE
                }
            } catch (e: Exception) {
                Log.e("VoiceFragment", "Failed to load portrait image from $relativePortraitPath", e)
                voicePortraitImage.visibility = View.GONE
            }
        } else {
            voicePortraitImage.visibility = View.GONE
        }
    }

    private fun deleteDirectoryContents(dir: File): Boolean {
        if (!dir.exists() || !dir.isDirectory) return false
        var success = true
        dir.listFiles()?.forEach { file -> success = success && deleteRecursively(file) }
        return success
    }

    private fun deleteRecursively(file: File): Boolean {
        return if (file.isDirectory) {
            file.listFiles()?.forEach { child -> if (!deleteRecursively(child)) return false }
            file.delete()
        } else {
            file.delete()
        }
    }

    private fun copyFilteredFilesRecursively(
        source: DocumentFile, destRoot: File, relativePath: String
    ) {
        for (file in source.listFiles()) {
            val newRelPath =
                if (relativePath.isEmpty()) file.name ?: "" else "$relativePath/${file.name}"
            if (file.isDirectory) {
                copyFilteredFilesRecursively(file, destRoot, newRelPath)
            } else if (file.isFile && isTargetExtension(requireNotNull(file.name))) {
                val destFile = File(destRoot, newRelPath)
                destFile.parentFile?.mkdirs()
                copyFile(file, destFile)
            }
        }
    }

    private fun copyFile(source: DocumentFile, dest: File) {
        try {
            requireActivity().contentResolver.openInputStream(source.uri)?.use { input ->
                FileOutputStream(dest).use { output -> input.copyTo(output) }
            }
        } catch (e: Exception) {
            Log.e("VoiceFragment", "Failed to copy ${source.name}", e)
        }
    }

    private fun isTargetExtension(name: String) =
        name.endsWith(".toml", ignoreCase = true) ||
        name.endsWith(".bin", ignoreCase = true) ||
        name.endsWith(".png", ignoreCase = true)
}
