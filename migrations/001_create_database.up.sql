CREATE TABLE IF NOT EXISTS "roles" (
  "id" bigserial PRIMARY KEY,
  "type" varchar(255) NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS "users" (
  "id" bigserial PRIMARY KEY,
  "role_id" bigint NOT NULL REFERENCES "roles" ("id"),
  "email" varchar(255) NOT NULL UNIQUE,
  "password" varchar(255) NOT NULL,
  "name" varchar(255) NOT NULL,
  "second_name" varchar(255) NOT NULL,
  "third_name" varchar(255)
);

CREATE TABLE IF NOT EXISTS "groups" (
  "name" varchar(255) PRIMARY KEY,
  "start_year" integer NOT NULL,
  "duration_years" integer NOT NULL CHECK ("duration_years" > 0)
);

CREATE TABLE IF NOT EXISTS "students" (
  "id" bigserial PRIMARY KEY,
  "user_id" bigint NOT NULL UNIQUE REFERENCES "users" ("id"),
  "group_name" varchar(255) NOT NULL REFERENCES "groups" ("name")
);

CREATE TABLE IF NOT EXISTS "subjects" (
  "id" bigserial PRIMARY KEY,
  "name" varchar(255) NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS "student_group_rel" (
  "student_id" bigint NOT NULL UNIQUE REFERENCES "users" ("id"),
  "group_name" varchar(255) NOT NULL REFERENCES "groups" ("name"),
  PRIMARY KEY ("student_id", "group_name")
);

CREATE TABLE IF NOT EXISTS "group_subject_rel" (
  "subject_id" bigint NOT NULL REFERENCES "subjects" ("id"),
  "group_name" varchar(255) NOT NULL REFERENCES "groups" ("name"),
  PRIMARY KEY ("subject_id", "group_name")
);

CREATE TABLE IF NOT EXISTS "lessons" (
  "id" bigserial PRIMARY KEY,
  "subject_id" bigint NOT NULL REFERENCES "subjects" ("id"),
  "teacher_id" bigint REFERENCES "users" ("id"),
  "group_name" varchar(255) NOT NULL REFERENCES "groups" ("name"),
  "lesson_date_start" timestamp NOT NULL,
  "lesson_duration_hours" integer NOT NULL CHECK ("lesson_duration_hours" > 0)
);

CREATE TABLE IF NOT EXISTS "lesson_attendance" (
  "lesson_id" bigint NOT NULL REFERENCES "lessons" ("id"),
  "student_id" bigint NOT NULL REFERENCES "students" ("id"),
  "attendance_status" varchar(255) NOT NULL CHECK (
    "attendance_status" IN ('present', 'absent', 'late')
  ),
  PRIMARY KEY ("lesson_id", "student_id")
);

CREATE OR REPLACE FUNCTION delete_user_cascade () RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM lessons WHERE teacher_id = OLD.id;
    DELETE FROM student_group_rel WHERE student_id = OLD.id;

    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_delete_user_cascade BEFORE DELETE ON users FOR EACH ROW
EXECUTE FUNCTION delete_user_cascade ();

CREATE OR REPLACE FUNCTION delete_group_cascade () RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM lesson_attendance la
    WHERE la.lesson_id IN (
        SELECT id FROM lessons WHERE group_name = OLD.name
    );

    DELETE FROM lessons WHERE group_name = OLD.name;

    DELETE FROM group_subject_rel WHERE group_name = OLD.name;

    DELETE FROM students WHERE group_name = OLD.name;

    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_delete_group_cascade BEFORE DELETE ON groups FOR EACH ROW
EXECUTE FUNCTION delete_group_cascade ();

CREATE OR REPLACE FUNCTION delete_subject_cascade () RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM group_subject_rel WHERE subject_id = OLD.id;

    DELETE FROM lessons WHERE subject_id = OLD.id;

    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_delete_subject_cascade BEFORE DELETE ON subjects FOR EACH ROW
EXECUTE FUNCTION delete_subject_cascade ();

CREATE OR REPLACE FUNCTION delete_lesson_cascade () RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM lesson_attendance where lesson_id = OLD.id;

    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_delete_lesson_cascade BEFORE DELETE ON lessons FOR EACH ROW
EXECUTE FUNCTION delete_lesson_cascade ();
