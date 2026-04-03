// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficSim/Public/Road/IntersectionNode.h"

#include "Components/SphereComponent.h"

AIntersectionNode::AIntersectionNode()
{
	PrimaryActorTick.bCanEverTick = false;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(200.0f);
	SphereComponent->SetCollisionProfileName(TEXT("NoCollision"));
	RootComponent = SphereComponent;
}
