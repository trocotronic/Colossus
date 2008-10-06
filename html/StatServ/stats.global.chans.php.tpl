<chart>

	<chart_border color='2756CE' top_thickness='2' bottom_thickness='2' left_thickness='2' right_thickness='2' />
	<chart_data>
		<row>
			<null/>
<?
			require("_mysql.php");
			$query = mysql_query("SELECT dia, canales FROM " . $db_pref . "stats WHERE DATE_SUB(CURDATE(),INTERVAL 1 MONTH) <= dia");
			$mf = 0;
			$i = 0;
			$avg = 0;
			while (($row = mysql_fetch_row($query)))
			{
				$tmp = sscanf($row[0], "%*d-%d-%d", &$m, &$d);
				if ($m != $mf)
				{
					echo "<string>" . $d . "/" . $m . "</string>\n";
					$mf = $m;
				}
				else
					echo "<string>" . $d . "</string>\n";
				$i++;
			}
			echo '</row><row><string>Maximo canales</string>';
			mysql_data_seek($query, 0);
			$umax = 0;
			while (($row = mysql_fetch_row($query)))
			{
				echo "<number>" . $row[1] . "</number>\n";
				$avg += $row[1];
				if ($row[1] > $umax)
					$umax = $row[1];
			}
			mysql_free_result($query);
			echo '</row><row><string>Media estadistica</string>';
			echo str_repeat("<number>" . ($avg/$i) . "</number>\n", $i);
			echo '</row><row><string>Canales actuales</string>';
			echo str_repeat("<number>". (@CHANS_CUR@) ."</number>\n", $i);
?>
		</row>
	</chart_data>

	<axis_category size='14' color='000000' alpha='50' font='arial' bold='true' skip='0' orientation='horizontal' />
	<axis_ticks value_ticks='true' category_ticks='true' major_thickness='2' minor_thickness='1' minor_count='1' major_color='2756CE' minor_color='222222' position='outside' />
	<axis_value min='0' max='<?= ($umax+1) ?>' font='arial' bold='true' size='10' color='000000' alpha='50' steps='<?= ($umax+1) ?>' prefix='' suffix='' decimals='0' separator='' show_min='true' />

	<chart_grid_h alpha='10' color='000000' thickness='1' type='solid' />
	<chart_grid_v alpha='10' color='000000' thickness='1' type='solid' />
	<chart_pref line_thickness='2' point_shape='none' fill_shape='false' />
	<chart_rect x='40' y='50' width='335' height='200' positive_color='4B71FF' positive_alpha='30' negative_color='ff0000' negative_alpha='10' />
	<chart_type>Line</chart_type>
	<chart_value position='cursor' size='12' color='ffffff' alpha='75' />
	<chart_transition type='drop' delay='0' duration='1' order='category' />

	<draw>
		<text color='000000' alpha='30' font='arial' rotation='0' bold='true' size='31' x='0' y='0' width='400' height='150' h_align='left' v_align='top'>estadisticas por canales</text>
		<text color='666666' alpha='15' font='arial' rotation='-90' bold='true' size='40' x='-10' y='328' width='300' height='150' h_align='center' v_align='top'>nº canales</text>
		<text color='000000' alpha='15' font='arial' rotation='0' bold='true' size='60' x='0' y='250' width='320' height='300' h_align='left' v_align='top'>dias</text>
	</draw>

	<legend_rect x='260' y='265' width='10' height='10' margin='10' />
	<legend_label bullet='circle' />
	<legend_transition type='slide_right' delay='0' duration='1' />

	<series_color>
		<color>F7ED25</color>
		<color>faf7b5</color>
		<color>b1a905</color>
	</series_color>

</chart>
