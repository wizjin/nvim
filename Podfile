source 'https://github.com/CocoaPods/Specs.git'

inhibit_all_warnings!

$project_name = 'NVim'
$osx_version = '14.0'

platform :osx, $osx_version

target $project_name do
end

post_install do |installer|
	installer.pods_project.targets.each do |target|
		target.build_configurations.each do |config|
			config.build_settings['CLANG_ANALYZER_LOCALIZABILITY_NONLOCALIZED'] = 'YES'
			if config.build_settings['MACOSX_DEPLOYMENT_TARGET'].to_f < $osx_version.to_f
				config.build_settings['MACOSX_DEPLOYMENT_TARGET'] = $osx_version
			end
		end
	end
end

