// license:BSD-3-Clause
// copyright-holders:Enik Land
/**********************************************************************

    Sega FM Sound Unit emulation


Release data from the Sega Retro project:

  Year: 1987    Country/region: JP    Model code: FM-70

Notes:

The FM Unit add-on was released for the Mark III, so compatible games were
released only for that console and the Japanese SMS, that has the FM chip
built-in. The games play FM sound only when detect the FM hardware. The normal
PSG sound is replaced, except when sampled voices, not supported by the FM
chip, are played. Some games released for western SMS versions also contain FM
code, that is played on a Mark III / SMSJ when connected through a cartridge
adapter.

The IO address map (read/write ports) is based on Charles MacDonald's analysis
of Enri's schematics of the FM unit hardware.

Any software that access controllers through IO ports $C0/$C1 instead $DC/$DD
(the game Fushigi no Oshiro Pit Pot is a known example) has control problems
when the FM Sound Unit is attached (real hardware behavior).

TODO:

- Confirm the assumption that PSG and FM sound are not mixed by the FM unit
and there is only FM sound when the unit is enabled. Two evidences that were
observed: every time a game with FM sound plays sampled voices through PSG,
the game disables/enables the FM chip before/after playing the PSG sound, and
a user reported, on the topic of the SMSPower Forums where the PSG/FM mixing
control of the SMSJ was discovered, that the hack that adds FM sound to Sonic
SMS version is not playing PSG sound on his Mark III with the FM unit.

**********************************************************************/

#include "fm_unit.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type SEGA_FM_UNIT = &device_creator<sega_fm_unit_device>;


static MACHINE_CONFIG_FRAGMENT( fm_config )
	MCFG_SOUND_ADD("ym2413", YM2413, XTAL_10_738635MHz/3)
	// if this output gain is changed, the gain set when unmute the output need
	// to be changed too, probably along the gain set for SMSJ/SMSKRFM drivers.
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, ":mono", 1.00)
MACHINE_CONFIG_END


machine_config_constructor sega_fm_unit_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( fm_config );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  sega_fm_unit_device - constructor
//-------------------------------------------------

sega_fm_unit_device::sega_fm_unit_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, SEGA_FM_UNIT, "Sega FM Sound Unit", tag, owner, clock, "sega_fm_unit", __FILE__),
	device_sg1000_expansion_slot_interface(mconfig, *this),
	m_ym(*this, "ym2413"),
	m_psg(*this, ":segapsg"),
	m_audio_control(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void sega_fm_unit_device::device_start()
{
	/* register for state saving */
	save_item(NAME(m_audio_control));
}


//-------------------------------------------------
//  peripheral_r - fm unit read
//-------------------------------------------------

READ8_MEMBER(sega_fm_unit_device::peripheral_r)
{
	// the value previously written to the control port is returned on all
	// active read offsets.
	if (offset <= 3)
	{
		return m_audio_control & 0x01;
	}
	// will not be called for other offsets.
	return 0xff;
}

//-------------------------------------------------
//  peripheral_w - fm unit write
//-------------------------------------------------

WRITE8_MEMBER(sega_fm_unit_device::peripheral_w)
{
	switch (offset)
	{
		case 0: // register port
			m_ym->write(space, 0, data & 0x3f);
			break;
		case 1: // data port
			m_ym->write(space, 1, data);
			break;
		case 2: // control port
		case 3: // mirror
			m_audio_control = data & 0x01;
			if (m_audio_control == 0x01)
			{
				m_ym->set_output_gain(ALL_OUTPUTS, 1.0);
				// assume the PSG output is muted when FM is active.
				// Out Run need this. Needs confirmation (see TODO).
				if (m_psg.found())
					m_psg->set_output_gain(ALL_OUTPUTS, 0.0);
			}
			else
			{
				m_ym->set_output_gain(ALL_OUTPUTS, 0.0);
				if (m_psg.found())
					m_psg->set_output_gain(ALL_OUTPUTS, 1.0);
			}
			break;
		default:
			break;
	}
}


bool sega_fm_unit_device::is_readable(UINT8 offset)
{
	return (offset <= 3) ? true : false;
}


bool sega_fm_unit_device::is_writeable(UINT8 offset)
{
	return (offset <= 3) ? true : false;
}
