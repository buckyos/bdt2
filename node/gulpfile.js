const gulp = require("gulp");
const ts = require("gulp-typescript");
const sourcemaps = require("gulp-sourcemaps");
const tsProject = ts.createProject("tsconfig.json");

gulp.task("compile", function() {
    return tsProject.src()
        .pipe(sourcemaps.init())
        .pipe(tsProject())
        .js
        .pipe(sourcemaps.write())
        .pipe(gulp.dest("dist"))
});

gulp.task("res", () => {
    [
        gulp.src(["./src/**/*.sql", "./src/**/*.js", "./src/**/*.d.ts", "./src/**/*.json"])
            .pipe(gulp.dest("./dist/src")),
        gulp.src(["./src/js-c/build/Debug/*.node"])
            .pipe(gulp.dest("./dist/src/js-c/build/Release")),
        gulp.src(["./test/**/*.sql", "./test/**/*.js", "./test/**/*.d.ts", "./test/**/*.json"])
            .pipe(gulp.dest("./dist/test")),
        gulp.src(["./demo/**/*.json"])
            .pipe(gulp.dest("./dist/demo")),
    ];
});

gulp.task("build", ["compile", "res"]);


