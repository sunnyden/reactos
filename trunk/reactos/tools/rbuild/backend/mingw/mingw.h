#ifndef MINGW_H
#define MINGW_H

#include "../backend.h"

#ifdef WIN32
	#define NUL "NUL"
#else
	#define NUL "/dev/null"
#endif

class Directory;
class MingwModuleHandler;

extern std::string
v2s ( const string_list& v, int wrap_at );

typedef std::map<std::string,Directory*> directory_map;


class Directory
{
public:
	std::string name;
	directory_map subdirs;
	Directory ( const std::string& name );
	void Add ( const char* subdir );
	void GenerateTree ( const std::string& parent,
	                    bool verbose );
	std::string EscapeSpaces ( std::string path );
	void CreateRule ( FILE* f,
	                  const std::string& parent );
private:
	bool mkdir_p ( const char* path );
	std::string ReplaceVariable ( std::string name,
	                              std::string value,
	                              std::string path );
	std::string GetEnvironmentVariable ( const std::string& name );
	void ResolveVariablesInPath ( char* buf,
	                              std::string path );
	bool CreateDirectory ( std::string path );
};


class MingwBackend : public Backend
{
public:
	MingwBackend ( Project& project,
	               Configuration& configuration );
	virtual ~MingwBackend ();
	virtual void Process ();
	std::string AddDirectoryTarget ( const std::string& directory,
	                                 Directory* directoryTree );
	std::string compilerPrefix;
	std::string compilerCommand;
	bool usePipe;
	Directory* intermediateDirectory;
	Directory* outputDirectory;
	Directory* installDirectory;
private:
	void CreateMakefile ();
	void CloseMakefile () const;
	void GenerateHeader () const;
	std::string GenerateIncludesAndDefines ( IfableData& data ) const;
	void GenerateProjectCFlagsMacro ( const char* assignmentOperation,
	                                  IfableData& data ) const;
	void GenerateGlobalCFlagsAndProperties ( const char* op,
	                                         IfableData& data ) const;
	void GenerateProjectGccOptionsMacro ( const char* assignmentOperation,
                                              IfableData& data ) const;
	void GenerateProjectGccOptions ( const char* assignmentOperation,
	                                 IfableData& data ) const;
	std::string GenerateProjectLFLAGS () const;
	void GenerateDirectories ();
	void GenerateGlobalVariables () const;
	bool IncludeInAllTarget ( const Module& module ) const;
	void GenerateAllTarget ( const std::vector<MingwModuleHandler*>& handlers ) const;
	std::string GetBuildToolDependencies () const;
	void GenerateInitTarget () const;
	void GenerateRegTestsRunTarget () const;
	void GenerateXmlBuildFilesMacro() const;
	std::string GetBin2ResExecutable ();
	void UnpackWineResources ();
	void GenerateTestSupportCode ();
	void GenerateProxyMakefiles ();
	void CheckAutomaticDependencies ();
	bool IncludeDirectoryTarget ( const std::string& directory ) const;
	bool TryToDetectThisCompiler ( const std::string& compiler );
	void DetectCompiler ();
	void DetectPipeSupport ();
	void DetectPCHSupport ();
	void ProcessModules ();
	std::string GetNonModuleInstallDirectories ( const std::string& installDirectory );
	std::string GetInstallDirectories ( const std::string& installDirectory );
	void GetNonModuleInstallFiles ( std::vector<std::string>& out ) const;
	void GetInstallFiles ( std::vector<std::string>& out ) const;
	void GetNonModuleInstallTargetFiles ( std::vector<std::string>& out ) const;
	void GetModuleInstallTargetFiles ( std::vector<std::string>& out ) const;
	void GetInstallTargetFiles ( std::vector<std::string>& out ) const;
	void OutputInstallTarget ( const std::string& sourceFilename,
	                           const std::string& targetFilename,
	                           const std::string& targetDirectory );
	void OutputNonModuleInstallTargets ();
	void OutputModuleInstallTargets ();
	std::string GetRegistrySourceFiles ();
	std::string GetRegistryTargetFiles ();
	void OutputRegistryInstallTarget ();
	void GenerateInstallTarget ();
	void GetModuleTestTargets ( std::vector<std::string>& out ) const;
	void GenerateTestTarget ();
	void GenerateDirectoryTargets ();
	FILE* fMakefile;
	bool use_pch;
};


class ProxyMakefile
{
public:
	ProxyMakefile ( const Project& project );
	~ProxyMakefile ();
	void GenerateProxyMakefiles ( bool verbose );
private:
	std::string GeneratePathToParentDirectory ( int numberOfParentDirectories );
	std::string GetPathToTopDirectory ( Module& module );
	void GenerateProxyMakefileForModule ( Module& module,
                                              bool verbose );
	const Project& project;
};

#endif /* MINGW_H */
