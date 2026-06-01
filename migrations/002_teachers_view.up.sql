CREATE OR REPLACE VIEW student_view AS
SELECT
  u.id,
  u.email,
  u.password,
  u.name,
  u.second_name,
  u.third_name,
  r.type as role,
  sgr.group_name,
  g.start_year,
  g.duration_years,
  EXTRACT(
    YEAR
    FROM
      CURRENT_DATE
  ) - g.start_year + 1 as current_year
FROM
  users u
  JOIN roles r ON u.role_id = r.id
  LEFT JOIN student_group_rel sgr ON u.id = sgr.student_id
  LEFT JOIN groups g ON sgr.group_name = g.name
WHERE
  r.type = 'student';

CREATE OR REPLACE VIEW teacher_view AS
SELECT
  u.id,
  u.email,
  u.password,
  u.name,
  u.second_name,
  u.third_name,
  r.type as role,
  ARRAY_AGG(DISTINCT s.id) as subjects_ids,
  ARRAY_AGG(DISTINCT s.name) as subjects_names,
  ARRAY_AGG(DISTINCT g.name) as groups_names
FROM
  users u
  JOIN roles r ON u.role_id = r.id
  LEFT JOIN lessons l ON u.id = l.teacher_id
  LEFT JOIN subjects s ON l.subject_id = s.id
  LEFT JOIN groups g ON l.group_name = g.name
WHERE
  r.type = 'teacher'
GROUP BY
  u.id,
  u.email,
  u.name,
  u.second_name,
  u.third_name,
  r.type;

CREATE OR REPLACE FUNCTION get_group_lessons_for_date (p_group_name varchar, p_date date) RETURNS TABLE (
  lesson_id bigint,
  subject_name varchar,
  start_time timestamp,
  duration_hours integer,
  teacher_name text
) AS $$
  BEGIN
      RETURN QUERY
      SELECT
          l.id,
          s.name,
          l.lesson_date_start,
          l.lesson_duration_hours,
          COALESCE(u.second_name || ' ' || u.name, 'Неизвестно') as teacher_name
      FROM
          lessons l
      JOIN
          subjects s ON l.subject_id = s.id
      LEFT JOIN
          users u ON l.teacher_id = u.id
      WHERE
          l.group_name = p_group_name
          AND l.lesson_date_start::date = p_date
      ORDER BY
          l.lesson_date_start ASC;
  END;
  $$ LANGUAGE plpgsql;
