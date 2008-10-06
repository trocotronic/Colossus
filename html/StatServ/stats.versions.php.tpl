<chart>

	<chart_data>
		<row>
			<null/>
			@VERSIONS_INICIO@
			<string>@VERSIONS_NOMBRE@</string>
			@VERSIONS_FIN@
		</row>
		<row>
			<string></string>
			@VERSIONS_INICIO@
			<number>@VERSIONS_VALOR@</number>
			@VERSIONS_FIN@
		</row>
	</chart_data>
	<chart_grid_h thickness='0' />
	<chart_pref rotation_x='60' />
	<chart_rect x='50' y='50' width='300' height='200' positive_alpha='0' />
	<chart_transition type='spin' delay='.5' duration='0.75' order='category' />
	<chart_type>3d pie</chart_type>
	<chart_value color='000000' alpha='65' font='arial' bold='true' size='10' position='inside' prefix='' suffix='' decimals='0' separator='' as_percentage='true' />

	<draw>
		<text color='000000' alpha='4' size='40' x='-50' y='260' width='500' height='50' h_align='center' v_align='middle'>mirc*xchat*irssi*bitchx</text>
		<text color='000000' alpha='30' font='arial' rotation='0' bold='true' size='30' x='0' y='0' width='400' height='150' h_align='left' v_align='top'>estadisticas por versiones</text>
	</draw>

	<legend_label layout='horizontal' bullet='circle' font='arial' bold='true' size='12' color='000000' alpha='40' />
	<legend_rect x='0' y='45' width='50' height='210' margin='10' fill_color='ffffff' fill_alpha='10' line_color='000000' line_alpha='0' line_thickness='0' />
	<legend_transition type='slide_left' delay='0' duration='1' />
</chart>
