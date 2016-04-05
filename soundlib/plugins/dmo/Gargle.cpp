/*
 * Gargle.cpp
 * ----------
 * Purpose: Implementation of the DMO Gargle DSP (for non-Windows platforms)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#if !defined(NO_PLUGINS) && defined(NO_DMO)
#include "../../Sndfile.h"
#include "Gargle.h"

OPENMPT_NAMESPACE_BEGIN

namespace DMO
{

IMixPlugin* Gargle::Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
//---------------------------------------------------------------------------------------------
{
	return new (std::nothrow) Gargle(factory, sndFile, mixStruct);
}


Gargle::Gargle(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
	: IMixPlugin(factory, sndFile, mixStruct)
//---------------------------------------------------------------------------------
{
	m_param[kGargleRate] = 0.02f;
	m_param[kGargleWaveShape] = 0.0f;
	RecalculateGargleParams();
	InsertIntoFactoryList();
}


void Gargle::Process(float *pOutL, float *pOutR, uint32 numFrames)
//----------------------------------------------------------------
{
	float *inL = m_mixBuffer.GetInputBuffer(0), *inR = m_mixBuffer.GetInputBuffer(1);
	const bool triangle = m_param[kGargleWaveShape] < 1.0f;

	while(numFrames > 0)
	{
		if(m_counter < m_periodHalf)
		{
			// First half of gargle period
			const uint32 remain = std::min(numFrames, m_periodHalf - m_counter);
			if(triangle)
			{
				const uint32 stop = m_counter + remain;
				const float factor = 1.0f / m_periodHalf;
				for(uint32 i = m_counter; i < stop; i++)
				{
					*pOutL++ += *inL++ * i * factor;
					*pOutR++ += *inR++ * i * factor;
				}
			} else
			{
				for(uint32 i = 0; i < remain; i++)
				{
					*pOutL++ += *inL++;
					*pOutR++ += *inR++;
				}
			}
			numFrames -= remain;
			m_counter += remain;
		} else
		{
			// Second half of gargle period
			const uint32 remain = std::min(numFrames, m_period - m_counter);
			if(triangle)
			{
				const uint32 stop = m_period - m_counter - remain;
				const float factor = 1.0f / m_periodHalf;
				for(uint32 i = m_period - m_counter; i > stop; i--)
				{
					*pOutL++ += *inL++ * i * factor;
					*pOutR++ += *inR++ * i * factor;
				}
			} else
			{
				pOutL += remain;
				pOutR += remain;
				inL += remain;
				inR += remain;

			}
			numFrames -= remain;
			m_counter += remain;
			if(m_counter >= m_period) m_counter = 0;
		}
	}
}


PlugParamValue Gargle::GetParameter(PlugParamIndex index)
//-------------------------------------------------------
{
	if(index < kEqNumParameters)
	{
		return m_param[index];
	}
	return 0;
}


void Gargle::SetParameter(PlugParamIndex index, PlugParamValue value)
//-------------------------------------------------------------------
{
	if(index < kEqNumParameters)
	{
		Limit(value, 0.0f, 1.0f);
		if(index == kGargleWaveShape && value < 1.0f)
			value = 0.0f;
		m_param[index] = value;
		RecalculateGargleParams();
	}
}


void Gargle::Resume()
//-------------------
{
	RecalculateGargleParams();
	m_counter = 0;
}


#ifdef MODPLUG_TRACKER

CString Gargle::GetParamName(PlugParamIndex param)
//------------------------------------------------
{
	switch(param)
	{
	case kGargleRate: return _T("Rate");
	case kGargleWaveShape: return _T("WaveShape");
	}
	return CString();
}


CString Gargle::GetParamLabel(PlugParamIndex param)
//-------------------------------------------------
{
	switch(param)
	{
	case kGargleRate: return _T("Hz");
	}
	return CString();
}


CString Gargle::GetParamDisplay(PlugParamIndex param)
//---------------------------------------------------
{
	CString s;
	switch(param)
	{
	case kGargleRate:
		s.Format("%d", RateInHertz());
		break;
	case kGargleWaveShape:
		return (m_param[param] < 0.5) ? _T("Triangle") : _T("Square");
	}
	return s;
}

#endif // MODPLUG_TRACKER


int Gargle::RateInHertz() const
//-----------------------------
{
	return std::max(1, Util::Round<int>(m_param[kGargleRate] * 1000.0f));
}


void Gargle::RecalculateGargleParams()
//------------------------------------
{
	m_period = m_SndFile.GetSampleRate() / RateInHertz();
	if(m_period < 2) m_period = 2;
	m_periodHalf = m_period / 2;
	LimitMax(m_counter, m_period);
}

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS && NO_DMO