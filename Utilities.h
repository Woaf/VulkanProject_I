//
// Created by Fazekas Bálint on 2021. 12. 30..
//

#pragma once

#ifndef VULKANPROJECT_I_UTILITIES_H
#define VULKANPROJECT_I_UTILITIES_H


struct QueueFamilyIndices {
	int graphicsFamily = -1;

	bool IsValid () {
		return graphicsFamily >= 0;
	}
};


#endif //VULKANPROJECT_I_UTILITIES_H
