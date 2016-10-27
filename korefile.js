var project = new Project('Game', __dirname);

project.addFile('Sources/**');
project.setDebugDir('Deployment');
project.cpp11 = true;

Project.createProject('Kore', __dirname).then((subproject) => {
    project.addSubProject(subproject);
    resolve(project);
});
