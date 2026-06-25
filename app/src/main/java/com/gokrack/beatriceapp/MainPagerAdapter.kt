package com.gokrack.beatriceapp

import androidx.fragment.app.FragmentActivity
import androidx.viewpager2.adapter.FragmentStateAdapter

class MainPagerAdapter(activity: FragmentActivity) : FragmentStateAdapter(activity) {
    override fun getItemCount() = 4

    override fun createFragment(position: Int) = when (position) {
        0 -> VoiceFragment()
        1 -> PitchFragment()
        2 -> GainFragment()
        3 -> SettingsFragment()
        else -> VoiceFragment()
    }
}
