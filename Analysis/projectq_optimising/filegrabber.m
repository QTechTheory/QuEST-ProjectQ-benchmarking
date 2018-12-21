(* ::Package:: *)

BeginPackage["fileGrabber`"]

getFilename::usage="getFilename[folder, framework, numThreads, circDepth, numQubits] returns the filename of the file containing the given data"

Begin["`Private`"]

getFilename[
	framework_String, folder_String, numThreads_Integer,
	circuitDepth_Integer, numQubits_Integer
] :=
	With[
		{prefixDir = If[
			framework == "QuEST",
			"C",
			"Python"]},
		StringJoin[
			prefixDir, "/",
			folder, "/",
			"ARC_", framework, "/",
			"threads", ToString[numThreads], "/",
			"depth", ToString[circuitDepth], "/",
			"qubits", ToString[numQubits], ".txt"
		]
	]
	
getFilename[
	framework_String, numThreads_Integer,
	circuitDepth_Integer, numQubits_Integer
] :=
	getFilename[framework, "results", numThreads, circuitDepth, numQubits]

End[]

EndPackage[]
